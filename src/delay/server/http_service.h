#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

#include <boost/signals2/signal.hpp>

#include "server/service.h"


namespace boost {
namespace asio {

class io_context;

} // namespace boost::asio
} // namespace boost

namespace seeder {

class HttpRequest {
public:
    virtual nlohmann::json parse_json_body() const = 0;
};

class HttpResponse {
public:
    virtual void set_status_ok() = 0;
    virtual void set_status_err() = 0;
    virtual void set_json_content(const nlohmann::json &in_content) = 0;

    void json_ok() {
        set_status_ok();
        set_json_content(nlohmann::json::object());
    }

    void json_ok(const nlohmann::json &j) {
        set_status_ok();
        set_json_content(j);
    }

    void json_err_message(const std::string &message) {
        nlohmann::json j;
        j["error_message"] = message;
        json_err(j);
    }

    void json_err(const nlohmann::json &j) {
        set_status_err();
        set_json_content(j);
    }
};

enum class HttpMethod {
    Get,
    Post
};


class AbstractWebSocketSender {
public:
    virtual ~AbstractWebSocketSender();

    virtual void send_message(const std::string &msg) = 0;

    void send_message(const nlohmann::json &j) {
        send_message(j.dump());
    }
};

class WebSocketChannel : public AbstractWebSocketSender {
public:
    virtual ~WebSocketChannel();
};

class WebSocketChannelGroup : public AbstractWebSocketSender {
public:
    virtual ~WebSocketChannelGroup();

    virtual void on_connect(std::function<void(WebSocketChannel &)> h) = 0;
};

class HttpService : public Service {
public:
    explicit HttpService();
    ~HttpService();

    SEEDER_SERVICE_STATIC_ID("http");

    void setup(ServiceSetupCtx &ctx) override;

    void start() override;

    void add_route(
        HttpMethod method,
        const std::string &path,
        std::function<void(const HttpRequest &, HttpResponse &)> handler
    );

    // 会使用 asio::post(weak_io_ctx.lock()) 在目标线程执行handler并等待结果
    void add_route_with_ctx(
        HttpMethod method,
        const std::string &path,
        std::function<void(const HttpRequest &, HttpResponse &)> handler,
        std::weak_ptr<boost::asio::io_context> weak_io_ctx
    );

    void add_mount(const std::string &url_path, const std::string &filesystem_path);

    boost::signals2::signal<void()> & get_pre_start_signal();

    [[nodiscard]]
    std::shared_ptr<WebSocketChannelGroup> add_ws_group(const std::string &path);

    [[nodiscard]]
    std::shared_ptr<WebSocketChannelGroup> add_ws_group_with_interval(
        const std::string &path, 
        std::chrono::milliseconds interval,
        std::function<void(WebSocketChannelGroup &)> handler
    );

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};


} // namespace seeder