#include "http_service.h"

#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <thread>

#include <hv/EventLoop.h>
#include <hv/WebSocketServer.h>
#include <hv/hlog.h>

#include <boost/asio.hpp>

#include "core/util/logger.h"
#include "server/config_service.h"
#include "util/io.h"
#include "util/json_util.h"
#include "core/util/thread.h"


namespace asio = boost::asio;

namespace seeder {

namespace {

struct HttpConfig {
    std::string host = "localhost";
    int port = 0;
    bool log_access = false;
    std::optional<int> worker_threads;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    HttpConfig,
    host,
    port,
    log_access,
    worker_threads
)

class HttpRequestImpl : public HttpRequest {
public:
    explicit HttpRequestImpl(::HttpRequest &in_req): req(in_req) {}

    nlohmann::json parse_json_body() const override {
        return nlohmann::json::parse(req.body);
    }

private:
    ::HttpRequest &req;
};

class HttpResponseImpl : public HttpResponse {
public:
    explicit HttpResponseImpl(::HttpResponse &in_res): res(in_res) {}

    void set_status_ok() {
        res.status_code = HTTP_STATUS_OK;
    }

    void set_status_err() {
        res.status_code = HTTP_STATUS_BAD_REQUEST;
    }

    void set_json_content(const nlohmann::json &in_content) {
        res.content_type = APPLICATION_JSON;
        res.body = in_content.dump();
    }

private:
    ::HttpResponse &res;
};

class WebSocketChannelHandler {
public:
    std::function<void(const WebSocketChannelPtr& channel)> on_open;
    std::function<void(const WebSocketChannelPtr& channel)> on_close;
};

class WebSocketChannelContext {
public:
    std::shared_ptr<WebSocketChannelHandler> handler;
};

class WebSocketChannelImpl : public WebSocketChannel {
public:
    explicit WebSocketChannelImpl(WebSocketChannelPtr in_channel): channel(in_channel) {}

    void send_message(const std::string &msg) override {
        channel->send(msg);
    }

    WebSocketChannelPtr channel;
};

class WebSocketChannelGroupImpl : public WebSocketChannelGroup {
public:
    std::unique_lock<std::mutex> acquire_lock() {
        return std::unique_lock<std::mutex>(mutex);
    }

    void add_channel(const WebSocketChannelPtr &channel) {
        channels.insert({channel->id(), channel});
        WebSocketChannelImpl channel_impl(channel);
        if (m_on_connect) {
            m_on_connect(channel_impl);
        }
    }

    void remove_channel(const WebSocketChannelPtr &channel) {
        channels.erase(channel->id());
    }

    template<class F>
    void foreach_channel(F &&f) {
        for (auto iter = channels.begin(); iter != channels.end(); ++iter) {
            f(iter->second);
        }
    }

    void send_message(const std::string &msg) override {
        auto lock = acquire_lock();
        foreach_channel([&] (const WebSocketChannelPtr& channel) {
            channel->send(msg);
        });
    }

    void on_connect(std::function<void(WebSocketChannel &)> h) override {
        m_on_connect = h;
    }

    std::mutex mutex;
    std::unordered_map<unsigned int, WebSocketChannelPtr> channels;
    std::function<void(WebSocketChannel &)> m_on_connect;
};

class WebSocketServerImpl {
public:
    void init() {
        logger = seeder::core::get_logger("http_ws");

        ws_service.onopen = [this] (const WebSocketChannelPtr& channel, const HttpRequestPtr& req) {
            if (log_access) {
                logger->debug("WebSocketServerImpl onopen path={}", req->Path());
            }
            // 初始化后不会再修改url_handler_map的内容，所以这里访问url_handler_map不需要加锁
            auto handler_iter = url_handler_map.find(req->Path());
            if (handler_iter != url_handler_map.end()) {
                auto ctx = channel->newContextPtr<WebSocketChannelContext>();
                ctx->handler = handler_iter->second;
                if (ctx->handler->on_open) {
                    ctx->handler->on_open(channel);
                }
            } else {
                logger->debug("close websocket no handler available path={}", req->Path());
                channel->close();
            }
        };
        ws_service.onclose = [this] (const WebSocketChannelPtr& channel) {
            auto ctx = channel->getContextPtr<WebSocketChannelContext>();
            if (ctx && ctx->handler && ctx->handler->on_close) {
                ctx->handler->on_close(channel);
            }
        };
    }

    std::shared_ptr<WebSocketChannelGroupImpl> add_path_group(const std::string &path) {
        if (url_handler_map.find(path) != url_handler_map.end()) {
            logger->error("duplicated path {}", path);
            throw std::runtime_error("add_path_group");
        }
        auto group = create_group(path);
        std::weak_ptr<WebSocketChannelGroupImpl> weak_group(group);
        auto handler = std::make_shared<WebSocketChannelHandler>();
        handler->on_open = [this, weak_group, path] (const WebSocketChannelPtr& channel) {
            auto group = weak_group.lock();
            if (!group) {
                logger->debug("WebSocketServerImpl on_open path={} group not available", path);
                channel->close();
            }
            auto channel_group_lock = group->acquire_lock();
            group->add_channel(channel);
        };
        handler->on_close = [this, weak_group] (const WebSocketChannelPtr& channel) {
            auto group = weak_group.lock();
            if (!group) {
                return;
            }
            auto channel_group_lock = group->acquire_lock();
            group->remove_channel(channel);
        };
        url_handler_map[path] = handler;
        return group;
    }

    std::shared_ptr<WebSocketChannelGroupImpl> add_path_group_with_interval(
        const std::string &path, 
        std::chrono::milliseconds interval_ms,
        std::function<void(WebSocketChannelGroup &)> handler
    ) {
        auto group = add_path_group(path);
        std::weak_ptr<WebSocketChannelGroupImpl> weak_group(group);
        event_loop->setInterval(interval_ms.count(), [this, handler, weak_group] (hv::TimerID timer_id) {
            auto group = weak_group.lock();
            if (group) {
                handler(*group);
            } else {
                event_loop->killTimer(timer_id);
            }
        });
        return group;
    }

    std::shared_ptr<WebSocketChannelGroupImpl> create_group(const std::string &name) {
        if (groups.find(name) != groups.end()) {
            logger->error("duplicated group {}", name);
            throw std::runtime_error("create_group");
        }
        std::shared_ptr<WebSocketChannelGroupImpl> group(new WebSocketChannelGroupImpl);
        groups[name] = group;
        return group;
    }

    bool log_access = false;
    std::shared_ptr<spdlog::logger> logger;
    std::unordered_map<std::string, std::shared_ptr<WebSocketChannelHandler>> url_handler_map;
    std::unordered_map<std::string, std::weak_ptr<WebSocketChannelGroupImpl>> groups; 
    hv::EventLoopPtr event_loop;
    hv::WebSocketService ws_service;
};
    
} // namespace seeder::{}

class HttpService::Impl {
public:
    Impl() {
        event_loop.reset(new hv::EventLoop());
        ws_impl.event_loop = event_loop;
        ws_impl.init();
    }

    void start() {
        logger = seeder::core::get_logger("http");

        http_config = config_proxy.try_get();

        if (!http_config.has_value() || http_config->port <= 0) {
            logger->info("http server disabled");
            return;
        }

        ws_server.reset(new hv::WebSocketServer);
        if (!http_config->host.empty()) {
            ws_server->setHost(http_config->host.c_str());
        }
        ws_server->port = http_config->port;
        ws_server->worker_threads = http_config->worker_threads.value_or(8);
        ws_impl.log_access = http_config->log_access;

        join_handle = seeder::JoinHandle(std::thread([this] {
            seeder::core::util::set_thread_name("http_server");
            logger->info("http server listen on {}:{}", http_config->host, http_config->port);

            on_pre_start();

            ws_server->registerHttpService(&http_service);
            ws_server->registerWebSocketService(&ws_impl.ws_service);
            ws_server->start();

            event_loop->run();
        }));
    }

    std::shared_ptr<ConfigService> config_service;
    std::shared_ptr<spdlog::logger> logger;
    std::optional<HttpConfig> http_config;
    ConfigCategoryProxy<HttpConfig> config_proxy;
    boost::signals2::signal<void()> on_pre_start;
    
    WebSocketServerImpl ws_impl;
    hv::HttpService http_service;
    std::shared_ptr<hv::WebSocketServer> ws_server;
    
    hv::EventLoopPtr event_loop;
    seeder::JoinHandle join_handle;
};

HttpService::HttpService(): p_impl(new Impl) {}

HttpService::~HttpService() {}

void HttpService::setup(ServiceSetupCtx &ctx) {
    hlog_disable();
    p_impl->config_service = ctx.get_service<ConfigService>();
    p_impl->config_proxy.init_add(*p_impl->config_service, "http");
}

void HttpService::start() {
    p_impl->start();
}

void HttpService::add_route(
    HttpMethod method,
    const std::string &path,
    std::function<void(const HttpRequest &, HttpResponse &)> handler
) {
    const char *method_str = nullptr;
    if (method == HttpMethod::Get) {
        method_str = "GET";
    } else if (method == HttpMethod::Post) {
        method_str = "POST";
    }
    p_impl->http_service.Handle(
        method_str,
        path.c_str(),
        [this, handler, method_str] (::HttpRequest* req, ::HttpResponse* resp) {
            HttpRequestImpl req_impl(*req);
            HttpResponseImpl resp_impl(*resp);
            try {
                handler(req_impl, resp_impl);
            } catch (std::exception &e) {
                p_impl->logger->error("{} {} exception: {}", method_str, req->path, e.what());
                resp->status_code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
                resp->SetBody("server error");
            }
            if (p_impl->http_config->log_access) {
                p_impl->logger->debug("{} {} status code {}", method_str, req->path, (int)resp->status_code);
            }
            return resp->status_code;
        }
    );
}

void HttpService::add_route_with_ctx(
    HttpMethod method,
    const std::string &path,
    std::function<void(const HttpRequest &, HttpResponse &)> handler,
    std::weak_ptr<boost::asio::io_context> weak_io_ctx
) {
    this->add_route(method, path, [this, handler, weak_io_ctx] (const HttpRequest &req, HttpResponse &res) {
        auto io_ctx = weak_io_ctx.lock();
        if (!io_ctx || io_ctx->stopped()) {
            res.json_err_message("service not available");
            return;
        }
        auto promise = std::make_shared<std::promise<void>>();
        std::future<void> future = promise->get_future();
        asio::post(*io_ctx, [handler, promise, &req, &res] () {
            try {
                handler(req, res);
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });
        future.get();
    });
}

void HttpService::add_mount(const std::string &url_path, const std::string &filesystem_path) {
    p_impl->http_service.Static(url_path.c_str(), filesystem_path.c_str());
}


boost::signals2::signal<void()> & HttpService::get_pre_start_signal() {
    return p_impl->on_pre_start;
}

std::shared_ptr<WebSocketChannelGroup> HttpService::add_ws_group(const std::string &path) {
    return p_impl->ws_impl.add_path_group(path);
}

std::shared_ptr<WebSocketChannelGroup> HttpService::add_ws_group_with_interval(
    const std::string &path, 
    std::chrono::milliseconds interval,
    std::function<void(WebSocketChannelGroup &)> handler
) {
    return p_impl->ws_impl.add_path_group_with_interval(path, interval, handler);
}

AbstractWebSocketSender::~AbstractWebSocketSender() {}

WebSocketChannel::~WebSocketChannel() {}

WebSocketChannelGroup::~WebSocketChannelGroup() {}

} // namespace seeder