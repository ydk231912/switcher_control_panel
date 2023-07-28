

#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/fmt/ranges.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <boost/system/error_code.hpp>

#include <boost/program_options.hpp>
#include <boost/process.hpp>
#include <boost/process/detail/child_decl.hpp>
#include <boost/program_options/variables_map.hpp>

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/executor_work_guard.hpp>

#include <boost/process/detail/on_exit.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>

#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

#include <httplib.h>

namespace po = boost::program_options;
namespace bp = boost::process;
namespace asio = boost::asio;
namespace fs = boost::filesystem;

using httplib::Request;
using httplib::Response;

namespace {


std::string read_file_to_string(const std::string &file_path) {
    std::string content;
    std::ifstream file_stream(file_path, std::ios_base::binary);
    file_stream.exceptions(std::ios::badbit | std::ios::failbit);
    file_stream.seekg(0, std::ios_base::end);
    auto size = file_stream.tellg();
    file_stream.seekg(0);
    content.resize(static_cast<size_t>(size));
    file_stream.read(&content[0], static_cast<std::streamsize>(size));
    return content;
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
            SPDLOG_WARN("failed to mv {} {}", file_path, old_file_path, ec.message());
            return;
        }
        fs::rename(new_file_path, file_path, ec);
        if (ec) {
            SPDLOG_WARN("failed to mv {} {} : {}", new_file_path, file_path, ec.message());
            return;
        }
        fs::remove(old_file_path, ec);
        if (ec) {
            SPDLOG_WARN("failed to rm {} : {}", old_file_path, ec.message());
            return;
        }
    } else {
        write_string_to_file_directly(file_path, content);
    }
}

std::string json_to_string(const Json::Value &v) {
    Json::FastWriter writer;
    return writer.write(v);
}

void json_load_string(Json::Value &value, const std::string &s) {
    Json::Reader reader;
    reader.parse(s, value);
}


class Monitor {
public:
    Monitor(int argc, char **argv) {
        logger = spdlog::stdout_logger_mt("monitor");
        handle_args(argc, argv);
    }

    void run() {
        try {
            load_config();
            start_http_server();
            start_ipg_process();
            start_ipg_health_check_thread();
            process_io();
        } catch (std::exception &e) {
            logger->error("run(): {}", e.what());
        }
    }

    ~Monitor() {
        this->stop();

        logger->debug("wait http server thread stop");
        http_server_thread.join();
        
        logger->debug("wait ipg_health_check_thread stop");
        ipg_health_check_thread.join();
    }
private:
    void handle_args(int argc, char **argv) {
        std::string log_level = "info";
        po::options_description desc("Allowed options");
        desc.add_options()
            ("monitor_http_port", po::value(&http_port)->required(), "monitor http port")
            ("monitor_log_level", po::value(&log_level), "monitor logging level")
            ("http_port", po::value(&ipg_http_port)->required(), "ipg http port")
            ("ipg_exe", po::value(&ipg_exe_path)->required(), "ipg executable path")
            // ("monitor_config_file", po::value(&monitor_config_file_path)->required(), "monitor config file path")
            ("config_file", po::value(&ipg_config_file_path)->required(), "ipg config file path");
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        ipg_args = po::collect_unrecognized(parsed.options, po::include_positional);
        po::store(parsed, po_vm);
        po::notify(po_vm);

        ipg_exe_path = fs::absolute(ipg_exe_path).string();
        // monitor_config_file_path = fs::absolute(monitor_config_file_path).string();
        ipg_config_file_path = fs::absolute(ipg_config_file_path).string();
        spdlog::set_level(spdlog::level::from_str(log_level));
        logger->set_level(spdlog::level::from_str(log_level));
    }

    void load_config() {
        Json::Value json;
        json_load_string(json, read_file_to_string(ipg_config_file_path));
        config_json["interfaces"] = json["interfaces"];
    }

    void start_http_server() {
        if (http_server.is_running()) {
            return;
        }
        http_server.set_logger([this](const Request& req, const Response& res) {
            logger->debug("http {} status code {}", req.path, res.status);
        });

        http_server.Get("/monitor-api/ipg_status", [this] (const Request& req, Response& res) {
            get_ipg_status(req, res);
        });

        http_server.Get("/monitor-api/config", [this] (const Request& req, Response& res) {
            get_config(req, res);
        });

        http_server.Post("/monitor-api/update_config", [this] (const Request& req, Response& res) {
            update_ipg_config(req, res);
        });

        http_server.Post("/monitor-api/start_ipg", [this] (const Request& req, Response& res) {
            start_ipg(req, res);
        });

        http_server.Post("/monitor-api/stop_ipg", [this] (const Request& req, Response& res) {
            stop_ipg(req, res);
        });

        http_server.Post("/monitor-api/stop", [this] (const Request& req, Response& res) {
            stop();
        });

        http_server.Post("/monitor-api/restart_ipg", [this] (const Request& req, Response& res) {
            restart_ipg(req, res);
        });

        http_server_thread = std::thread([&] {
            const std::string host = "0.0.0.0";
            logger->info("http server listen on {}:{}", host, http_port);
            http_server.listen(host, http_port);
            logger->info("http server stopped");
        });
    }

    void start_ipg_process() {
        if (ipg_process.valid() && ipg_process.running()) {
            logger->error("start_ipg_process(): ipg is running");
            return;
        }
        auto actual_args = ipg_args;
        actual_args.emplace_back("--http_port");
        actual_args.emplace_back(std::to_string(ipg_http_port));
        actual_args.emplace_back("--config_file");
        actual_args.emplace_back(ipg_config_file_path);

        logger->info("start_ipg_process(): ipg_exe_path={} args={}", ipg_exe_path, spdlog::fmt_lib::join(actual_args, " "));
        auto old_pid = std::make_shared<bp::pid_t>(ipg_process.id());
        ipg_process = bp::child(ipg_exe_path, actual_args, io_context, bp::on_exit([this, old_pid](int exit, const std::error_code& ec) {
            logger->info("process exited with code {}", exit);
            if (ec) {
                logger->error("on_exit error: {}", ec.message());
            }
            // restart 时，进程结束的事件可能晚于process_status = ProcessStatus::STARTED，因此要判断id是否相同
            if (*old_pid == ipg_process.id()) {
                process_status = ProcessStatus::STOPPED;
            }
        }));
        logger->info("launch new process, pid: {}", ipg_process.id());
        *old_pid = ipg_process.id();
        process_status = ProcessStatus::STARTED;
    }

    void stop_ipg_process() {
        if (!ipg_process.valid() || !ipg_process.running()) {
            logger->error("stop_ipg_process(): ipg is not running");
            return;
        }
        ipg_process.terminate();
        ipg_process.wait();
    }

    void process_io() {
        auto work_guard = asio::make_work_guard(io_context); // run forever, until io_context.stop() is called
        io_context.run();
    }

    void save_config() {
        try {
            Json::Value current_config_json;
            json_load_string(current_config_json, read_file_to_string(ipg_config_file_path));
            for (int i = 0; i < config_json["interfaces"].size(); ++i) {
                current_config_json["interfaces"][i] = config_json["interfaces"][i];
            }
            write_string_to_file(ipg_config_file_path, json_to_string(current_config_json));
            logger->info("save config {}", ipg_config_file_path);
        } catch (std::exception &e) {
            logger->error("save_config(): {}", e.what());
        }
    }

    void update_ipg_config(const Request &req, Response &res) {
        auto json_body = parse_json_body(req);
        auto &interfaces = json_body["interfaces"];
        if (interfaces.empty()) {
            json_err(res, "interfaces is required");
            return;
        }
        if (interfaces.size() != config_json["interfaces"].size()) {
            json_err(res, "interfaces.size mismatch");
            return;
        }
        std::unordered_set<std::string> ip_map;
        for (auto &inf : interfaces) {
            if (inf["ip"].isString() && !inf["ip"].asString().empty()) {
                boost::system::error_code ec;
                auto ip = inf["ip"].asString();
                asio::ip::make_address_v4(ip, ec);
                if (ec) {
                    json_err(res, ec.message());
                    return;
                }
                if (ip_map.find(ip) != ip_map.end()) {
                    json_err(res, "IP Address Conflict");
                    return;
                } else {
                    ip_map.insert(ip);
                }
            }
        }
        if (ip_map.empty()) {
            json_err(res, "IP Address is required");
            return;
        }
        
        auto lock = acquire_lock();
        for (int i = 0; i < interfaces.size(); ++i) {
            auto &inf = interfaces[i];
            config_json["interfaces"][i]["ip"] = inf["ip"];
        }
        save_config();
        json_ok(res);
    }

    void get_ipg_status(const Request &req, Response &res) {
        Json::Value value;
        value["process_status"] = static_cast<int>(process_status.load());
        json_ok(res, value);
    }

    void get_config(const Request &req, Response &res) {
        Json::Value config;
        {
            auto lock = acquire_lock();
            config = config_json;
        }
        auto &interfaces = config["interfaces"];
        for (int i = 0; i < interfaces.size(); ++i) {
            interfaces[i]["display_name"] = "WAN" + std::to_string(i + 1);
        }
        json_ok(res, config);
    }

    void start_ipg(const Request &req, Response &res) {
        auto lock = acquire_lock();
        if (!(process_status == ProcessStatus::STARTED || process_status == ProcessStatus::RUNNING)) {
            logger->info("start ipg request by user");
            start_ipg_process();
        }
        json_ok(res);
    }

    void stop_ipg(const Request &req, Response &res) {
        auto lock = acquire_lock();
        if (process_status == ProcessStatus::STARTED || process_status == ProcessStatus::RUNNING) {
            logger->info("stop ipg request by user");
            stop_ipg_process();
        }
        json_ok(res);
    }

    void restart_ipg(const Request &req, Response &res) {
        auto lock = acquire_lock();
        logger->info("restart ipg request by user");
        stop_ipg_process();
        start_ipg_process();
        json_ok(res);
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

    std::unique_lock<std::mutex> acquire_lock() {
        return std::unique_lock<std::mutex>{mutex};
    }

    void start_ipg_health_check_thread() {
        ipg_health_check_thread = std::thread([this] {
            logger->info("ipg_health_check_thread start");
            std::string host = "http://localhost";
            auto url = host + ":" + std::to_string(ipg_http_port);
            while (!stopped) {
                httplib::Client client(url);
                client.set_connection_timeout(5, 0);
                client.set_read_timeout(5, 0); // 5 seconds
                client.set_write_timeout(5, 0); // 5 seconds
                if (auto res = client.Get("/api/health")) {
                    if (res->status == 200) {
                        auto expected = ProcessStatus::STARTED;
                        process_status.compare_exchange_strong(expected, ProcessStatus::RUNNING);
                    }
                } else {
                    logger->debug("start_ipg_health_check_thread client.Get(): {} {}", url, httplib::to_string(res.error()));
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            logger->info("ipg_health_check_thread finish");
        });
    }

    void stop() {
        bool expected = false;
        if (!stopped.compare_exchange_strong(expected, true)) {
            return;
        }
        logger->info("stopping");

        http_server.stop();
        if (ipg_process.valid() && ipg_process.running()) {
            ipg_process.terminate();
            ipg_process.wait();
        }

        io_context.stop();
        logger->info("stopped");
    }
private:
    enum class ProcessStatus {
        NOT_RUNNING, // 初始状态，未运行
        STARTED, // 已启动
        RUNNING, // 正在运行
        STOPPED // 已停止
    };
private:
    std::shared_ptr<spdlog::logger> logger;
    po::variables_map po_vm;
    std::string ipg_exe_path;
    // std::string monitor_config_file_path;
    std::string ipg_config_file_path;
    std::vector<std::string> ipg_args;
    int http_port;
    int ipg_http_port;
    bp::child ipg_process;
    asio::io_context io_context;
    Json::Value config_json;
    std::mutex mutex;
    std::atomic<ProcessStatus> process_status = ProcessStatus::NOT_RUNNING;
    std::atomic<bool> stopped = false;
    httplib::Server http_server;
    std::thread http_server_thread;
    std::thread ipg_health_check_thread;
};


} // namespace

int main(int argc, char **argv) {
    try {
        Monitor monitor(argc, argv);
        monitor.run();
    } catch (std::exception &e) {
        spdlog::error("main exception: {}", e.what());
    }
    
    return 0;
}