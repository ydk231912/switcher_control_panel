#pragma once 

#include <httplib.h>
#include <thread>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "../util/os.h"

namespace seeder {

extern std::shared_ptr<spdlog::logger> logger;

class HttpServer {
public:
    HttpServer(const std::string &host_, int port_);
    ~HttpServer();

    void init();
    void start();
    void stop();

    std::shared_ptr<httplib::Server> http_server;

    std::unique_lock<std::recursive_mutex> acquire_lock() {
        return std::unique_lock<std::recursive_mutex>(mutex);
    }
    std::recursive_mutex mutex;
private:
    std::string host = "";
    int port = 0;
    std::atomic<bool> ser_run;

    std::thread server_thread;
};
} // namespace seeder