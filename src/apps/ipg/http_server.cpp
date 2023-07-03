#include "http_server.h"
#include "core/util/logger.h"
#include <boost/filesystem/operations.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include "httplib.h"
#include <ios>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <json/json.h>

using httplib::Request;
using httplib::Response;

#define SAFE_LOG(level, ...) if (seeder::core::logger) (seeder::core::logger)->level(__VA_ARGS__)

namespace {
    namespace fs = boost::filesystem;

    void read_file_to_string(const std::string &file_path, std::string &output) {
        std::fstream stream(file_path, std::ios_base::in);
        std::string buf;
        while (stream >> buf) {
            output.append(buf);
        }
    }

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
        std::string output;
        read_file_to_string(app_ctx->config_file_path, output);
        json_ok(res, output);
    }

    void save_config(const Request& req, Response& res) {
        if (req.body.empty()) {
            text_err(res, "empty body");
            return;
        }
        write_string_to_file(app_ctx->config_file_path, req.body);
        logger->info("save config file {}", app_ctx->config_file_path);
    }

    void add_tx_session(const Request& req, Response& res) {
        Json::Reader json_reader;
        Json::FastWriter write;
        Json::Value root;
        json_reader.parse(req.body, root);
        Json::Value add_tx;
        Json::Value root_tx_arr = root["tx_sessions"];

        // int count = root_tx_arr.size();
        
        // Json::Value add_tx_array =  add_tx["tx_sessions"];
        // for (size_t i = 0; i < add_tx_array.size(); i++)
        // {
        //     count = count + i;
        //     Json::Value add_tx_obj = add_tx_array[i];
        //     add_tx_obj["id"] = count;
        //     root_tx_arr[count] = add_tx_obj;
        // }

        // root["tx_sessions"] = root_tx_arr;
        // res.body = write.write(root);

        // st_json_context_t * ctx_add = (st_json_context_t*)st_app_zmalloc(sizeof(st_json_context_t));
        // ret = st_app_parse_json_object_add_tx(ctx_add, add_tx_array,root,count);
        // ret = st_tx_video_source_init_add(app_ctx, ctx_add);
        // ret = st_app_tx_video_sessions_init_add(app_ctx, ctx_add);
        // ret = st_app_tx_audio_sessions_init_add(app_ctx, ctx_add);
        json_ok(res);
    }
    
    void update_tx_session(const Request& req, Response& res) {
        //             for (size_t i = 0; i < update_tx_array.size(); i++)
//             {
//             Value update_tx_obj = update_tx_array[i];
//             int id = update_tx_obj["id"];
//             ret = st_tx_video_source_init_update( ctx,ctx_update,id);
//             ret = st_app_tx_video_sessions_uinit_update(ctx_update,id,ctx_update);
//             ret = st_app_tx_audio_sessions_uinit_update(ctx_update,id,ctx_update); 
//             } 
    }

    static void json_ok(Response& res) {
        res.status = 200;
        res.set_content("{}", "application/json");
    }
    
    static void json_ok(Response& res, const std::string &json_content) {
        res.status = 200;
        res.set_content(json_content, "application/json");
    }

    static void text_err(Response& res, const std::string &message) {
        res.status = 400;
        res.set_content(message, "text/plain");
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
    }

    server.set_logger([&p](const Request& req, const Response& res) {
        p.logger->debug("http {} status code {}", req.path, res.status);
    });

    server.Get("/api/json", [&p] (const Request& req, Response& res) {
        p.get_config(req, res);
    });

    server.Post("/api/saveConfig", [&p] (const Request& req, Response& res) {
        p.save_config(req, res);
    });

    server.Post("/api/add_tx_json", [&p] (const Request& req, Response& res) {
        p.add_tx_session(req, res);
    });

    server.Post("/api/update_tx_json", [&p] (const Request& req, Response& res) {
        p.update_tx_session(req, res);
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