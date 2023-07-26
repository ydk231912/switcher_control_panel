#include "http_server.h"
#include "app_error_code.h"
#include "core/util/logger.h"
#include <boost/filesystem/operations.hpp>
#include <boost/system/error_code.hpp>
#include <exception>
#include <functional>
#include "httplib.h"
#include <ios>
#include <json/forwards.h>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <core/util/fs.h>
#include "parse_json.h"
#include "app_session.h"

using httplib::Request;
using httplib::Response;

#define SAFE_LOG(level, ...) if (seeder::core::logger) (seeder::core::logger)->level(__VA_ARGS__)

namespace {
    namespace fs = boost::filesystem;

    void write_string_to_file_directly(const std::string &file_path, const std::string &content) {
        std::fstream stream(file_path, std::ios_base::out);
        stream << content;
        stream.flush();
    }

    void write_string_to_file(const std::string &file_path, const std::string &content) {
        if (fs::exists(file_path)) {
            boost::system::error_code ec;
            std::string new_file_path = file_path + ".new";
            write_string_to_file_directly(new_file_path, content);
            std::string old_file_path = file_path + ".old";
            fs::rename(file_path, old_file_path, ec);
            if (ec) {
                SAFE_LOG(warn, "failed to mv {} {}", file_path, old_file_path, ec.message());
                return;
            }
            fs::rename(new_file_path, file_path, ec);
            if (ec) {
                SAFE_LOG(warn, "failed to mv {} {} : {}", new_file_path, file_path, ec.message());
                return;
            }
            fs::remove(old_file_path, ec);
            if (ec) {
                SAFE_LOG(warn, "failed to rm {} : {}", old_file_path, ec.message());
                return;
            }
        } else {
            write_string_to_file_directly(file_path, content);
        }
    }

    std::string read_file_to_string(const std::string &file_path) {
        std::string content;
        std::ifstream file_stream(file_path, std::ios_base::binary);
        // file_stream.exceptions(std::ios::badbit | std::ios::failbit);
        file_stream.seekg(0, std::ios_base::end);
        auto size = file_stream.tellg();
        file_stream.seekg(0);
        content.resize(static_cast<size_t>(size));
        file_stream.read(&content[0], static_cast<std::streamsize>(size));
        return content;
    }
}

namespace seeder {

class st_http_server::impl : boost::noncopyable {
friend class st_http_server;
private:
    st_app_context *app_ctx;
    httplib::Server server;
    std::thread server_thread;
    std::shared_ptr<spdlog::logger> logger;

    void get_config(const Request& req, Response& res) {
        Json::Value root;
        {
            auto lock = acquire_ctx_lock();
            root.copy(app_ctx->json_ctx->json_root);
        }
        json_ok(res, root);
    }

    void get_health(const Request& req, Response& res) {
        json_ok(res);
    }

    void save_config(const Request& req, Response& res) {
        if (req.body.empty()) {
            json_err(res, "empty body");
            return;
        }
        auto lock = acquire_ctx_lock();
        write_string_to_file(app_ctx->config_file_path, req.body);
        logger->info("save config file {}", app_ctx->config_file_path);
    }

    void get_fmts(const Request& req, Response& res) {
        auto json_value = st_app_get_fmts(app_ctx->json_ctx.get());
        json_ok(res, json_value);
    }

    void add_session(const Request& req, Response& res) {
        auto lock = acquire_ctx_lock();
        std::unique_ptr<st_json_context_t> new_ctx;
        std::error_code ec {};
        ec = st_app_parse_json_add(app_ctx->json_ctx.get(), req.body, new_ctx);
        if (ec) {
            logger->warn("st_app_parse_json_add error {} {}", ec.value(), ec.message());
            json_err(res, ec);
            return;
        }
        ec = st_app_add_tx_sessions(app_ctx, new_ctx.get());
        if (ec) {
            logger->warn("st_app_add_tx_sessions error {} {}", ec.value(), ec.message());
            json_err(res, ec);
            return;
        }
        ec = st_app_add_rx_sessions(app_ctx, new_ctx.get());
        if (ec) {
            logger->warn("st_app_add_rx_sessions error {} {}", ec.value(), ec.message());
            json_err(res, ec);
            return;
        }
        save_config_file();
        json_ok(res);
    }
    
    void update_session(const Request& req, Response& res) {
        auto root = parse_json_body(req);
        if (!check_require_tx_source_id(root)) {
            json_err(res, "check_require_tx_source_id error");
            return;
        }
        if (!check_require_rx_output_id(root)) {
            json_err(res, "check_require_rx_output_id error");
            return;
        }
        auto lock = acquire_ctx_lock();
        std::unique_ptr<st_json_context_t> new_ctx;
        std::error_code ec {};
        ec = st_app_parse_json_update(app_ctx->json_ctx.get(), req.body, new_ctx);
        if (ec) {
            logger->warn("st_app_parse_json_update error {} {}", ec.value(), ec.message());
            json_err(res, ec);
            return;
        }
        ec = st_app_update_tx_sessions(app_ctx, new_ctx.get());
        if (ec) {
            logger->warn("st_app_update_tx_sessions error {} {}", ec.value(), ec.message());
            json_err(res, ec);
            return;
        }
        ec = st_app_update_rx_sessions(app_ctx, new_ctx.get());
        if (ec) {
            logger->warn("st_app_update_rx_sessions error {} {}", ec.value(), ec.message());
            json_err(res, ec);
            return;
        }
        save_config_file();
        json_ok(res);
    }

    void remove_tx_session(const Request& req, Response& res) {
        auto root = parse_json_body(req);
        auto tx_source_id = root["tx_source_id"].asString();
        if (tx_source_id.empty()) {
            json_err(res, "tx_source_id empty");
            return;
        }
        auto lock = acquire_ctx_lock();
        std::error_code ec {};
        ec = st_app_remove_tx_session(app_ctx, tx_source_id);
        if (ec) {
            logger->warn("st_app_remove_tx_session error {} {}", ec.value(), ec.message());
            json_err(res, "st_app_remove_tx_session error");
            return;
        }
        save_config_file();
        json_ok(res);
    }

    void remove_rx_session(const Request& req, Response& res) {
        auto root = parse_json_body(req);
        auto rx_output_id = root["rx_output_id"].asString();
        if (rx_output_id.empty()) {
            json_err(res, "rx_output_id empty");
            return;
        }
        auto lock = acquire_ctx_lock();
        std::error_code ec {};
        ec = st_app_remove_rx_session(app_ctx, rx_output_id);
        if (ec) {
            logger->warn("st_app_remove_rx_session error {} {}", ec.value(), ec.message());
            json_err(res, "st_app_remove_rx_session error");
            return;
        }
        save_config_file();
        json_ok(res);
    }

    void get_stat(const Request& req, Response& res) {
        Json::Value stat;
        {
            auto lock = acquire_ctx_lock();
            stat = app_ctx->stat;
        }
        json_ok(res, stat);
    }

    void save_config_file() {
        try {
            Json::Reader reader;
            Json::Value config_json;
            reader.parse(read_file_to_string(app_ctx->config_file_path), config_json);
            app_ctx->json_ctx->json_root["interfaces"] = config_json["interfaces"];
            Json::FastWriter writer;
            std::string output = writer.write(app_ctx->json_ctx->json_root);
            write_string_to_file(app_ctx->config_file_path, output);
            logger->info("save config file {}", app_ctx->config_file_path);
        } catch (std::exception &e) {
            logger->error("save_config_file(): {}", e.what());
        }
    }

    bool check_require_tx_source_id(Json::Value &root) {
        auto &tx_sessions = root["tx_sessions"];
        if (!tx_sessions.isArray()) {
            return true;
        }
        for (Json::ArrayIndex i = 0; i < tx_sessions.size(); ++i) {
            auto &s = tx_sessions[i];
            if (!s["id"].isString() || s["id"].asString().empty()) {
                return false;
            }
        }
        return true;
    }

    bool check_require_rx_output_id(Json::Value &root) {
        auto &rx_sessions = root["rx_sessions"];
        if (!rx_sessions.isArray()) {
            return true;
        }
        for (Json::ArrayIndex i = 0; i < rx_sessions.size(); ++i) {
            auto &s = rx_sessions[i];
            if (!s["id"].isString() || s["id"].asString().empty()) {
                return false;
            }
        }
        return true;
    }

    std::unique_lock<std::mutex> acquire_ctx_lock() {
        return std::unique_lock<std::mutex>(app_ctx->mutex);
    }

    static void json_ok(Response& res) {
        res.status = 200;
        res.set_content("{}", "application/json");
    }
    
    static void json_ok(Response& res, const std::string &json_content) {
        res.status = 200;
        res.set_content(json_content, "application/json");
    }

    static void json_ok(Response& res, const Json::Value &value) {
        Json::FastWriter writer;
        std::string output = writer.write(value);
        json_ok(res, output);
    }

    static void text_err(Response& res, const std::string &message) {
        res.status = 400;
        res.set_content(message, "text/plain");
    }

    static void text_err(Response& res, const std::string &message, const std::error_code &ec) {
        text_err(res, message + ": " + ec.message());
    }

    static void json_err_content(Response& res, const Json::Value &value) {
        Json::FastWriter writer;
        std::string output = writer.write(value);
        res.status = 400;
        res.set_content(output, "application/json");
    }

    static void json_err(Response& res, const std::string &message) {
        Json::Value root;
        root["error_code"] = 0;
        root["error_message"] = message;
        json_err_content(res, root);
    }

    static void json_err(Response& res, const std::error_code &ec) {
        Json::Value root;
        root["error_code"] = ec.value();
        root["error_category"] = ec.category().name();
        root["error_message"] = ec.message();
        json_err_content(res, root);
    }

    static Json::Value parse_json_body(const Request req) {
        return parse_json(req.body);
    }

    static Json::Value parse_json(const std::string &json) {
        Json::Reader json_reader;
        Json::Value value;
        json_reader.parse(json, value);
        return value;
    }
};

st_http_server::st_http_server(st_app_context *app_ctx): p_impl(new impl) {
    p_impl->app_ctx = app_ctx;
    p_impl->logger = seeder::core::logger;
}

st_http_server::~st_http_server() {}

void st_http_server::start() {
    auto &server = p_impl->server;
    impl &p = *p_impl;

    if (server.is_running()) {
        return;
    }
    if (p.app_ctx->http_port <= 0) {
        p.logger->warn("http_port={} http server disabled", p.app_ctx->http_port);
        return;
    }

    server.set_logger([&p](const Request& req, const Response& res) {
        p.logger->debug("http {} status code {}", req.path, res.status);
    });

    server.Get("/api/json", [&p] (const Request& req, Response& res) {
        p.get_config(req, res);
    });

    server.Get("/api/get_fmt", [&p] (const Request& req, Response& res) {
        p.get_fmts(req, res);
    });

    server.Get("/api/health", [&p] (const Request& req, Response& res) {
        p.get_health(req, res);
    });

    server.Post("/api/saveConfig", [&p] (const Request& req, Response& res) {
        p.save_config(req, res);
    });

    server.Post("/api/add_tx_json", [&p] (const Request& req, Response& res) {
        p.add_session(req, res);
    });

    server.Post("/api/update_tx_json", [&p] (const Request& req, Response& res) {
        p.update_session(req, res);
    });

    server.Post("/api/remove_tx_json", [&p] (const Request& req, Response& res) {
        p.remove_tx_session(req, res);
    });

    server.Post("/api/add_rx_json", [&p] (const Request& req, Response& res) {
        p.add_session(req, res);
    });

    server.Post("/api/update_rx_json", [&p] (const Request& req, Response& res) {
        p.update_session(req, res);
    });

    server.Post("/api/remove_rx_json", [&p] (const Request& req, Response& res) {
        p.remove_rx_session(req, res);
    });

    server.Get("/api/stat", [&p] (const Request& req, Response& res) {
        p.get_stat(req, res);
    });

    p.server_thread = std::thread([&] {
        const std::string host = "0.0.0.0";
        int port = p.app_ctx->http_port;
        p.logger->info("http server listen on {}:{}", host, port);
        server.listen(host, port);
        p.logger->info("http server stopped");
    });
}

void st_http_server::stop() {
    p_impl->server.stop();
}

} // namespace seeder