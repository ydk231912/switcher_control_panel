#include "http_server.h"

namespace seeder {
HttpServer::HttpServer(const std::string &host_, int port_)
    : http_server(new httplib::Server), host(host_), port(port_), ser_run(false), server_thread() {
  init();
}

HttpServer::~HttpServer() {
    stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }
}

void HttpServer::init() {
    http_server->Get("/api/armpanel/poweroff", [this](const httplib::Request& req, httplib::Response& res) {
        seeder::util::restart();
        return 200;
    });
    http_server->Get("/api/armpanel/restart", [this](const httplib::Request& req, httplib::Response& res) {
        seeder::util::restart();
        return 200;
    });
    logger->info("HTTP Server started on {}:{}", host, port);
}

void HttpServer::start() {
    ser_run = true;
    server_thread = std::thread([this] {
        int retry_count = 0;
        const int retry_interval = 3;
        while (true) {
            if (http_server->listen(host, port)) {
                logger->info("HTTP Server started on {}:{}", host, port);
                break;
            } else {
                logger->error("Failed to start HTTP Server on {}:{}, retrying... (Attempt #{})", host, port, ++retry_count);
                std::this_thread::sleep_for(std::chrono::seconds(retry_interval));
            }
        }
    });
}

void HttpServer::stop() {
    if (ser_run) {
        ser_run = false;
        http_server->stop();
        if (server_thread.joinable()) {
        server_thread.join();
        }
        logger->info("HTTP Server stopped on {}:{}", host, port);
    } else {
        logger->warn("HTTP Server already stopped on {}:{}", host, port);
    }
}
} // namespace seeder