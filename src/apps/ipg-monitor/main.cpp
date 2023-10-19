

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
#include <spdlog/sinks/stdout_color_sinks.h>

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
        std::vector<spdlog::sink_ptr> sinks;
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinks.push_back(console_sink);
        logger = std::make_shared<spdlog::logger>("monitor", sinks.begin(), sinks.end());
        handle_args(argc, argv);

        nmcli_path = bp::search_path("nmcli");
        poweroff_path = bp::search_path("poweroff");
    }

    void run() {
        try {
            start_cmd_thread();
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

        logger->debug("wait cmd_thread stop");
        cmd_thread.join();

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
            ("config_file", po::value(&ipg_config_file_path)->required(), "ipg config file path")
            ("enable_auto_restart", po::value(&enable_auto_restart), "enable auto restart ipg");
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
        config_json["lan_interfaces"] = json["lan_interfaces"];

        load_lan_interfaces_info();
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

        http_server.Get("/monitor-api/lan_config", [this] (const Request& req, Response& res) {
            get_lan_interfaces(req, res);
        });

        http_server.Post("/monitor-api/update_lan_config", [this] (const Request& req, Response& res) {
            update_lan_interfaces_addr(req, res);
        });

        http_server.Post("/monitor-api/poweroff", [this] (const Request& req, Response& res) {
            poweroff(req, res);
        });

        http_server_thread = std::thread([&] {
            const std::string host = "0.0.0.0";
            logger->info("http server listen on {}:{}", host, http_port);
            http_server.listen(host, http_port);
            logger->info("http server stopped");
        });
    }

    void start_ipg_process() {
        if ((ipg_process.valid() && ipg_process.running()) || process_status == ProcessStatus::STARTED) {
            logger->info("start_ipg_process(): ipg is already running");
            return;
        }
        if (stopped) {
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

    void check_and_auto_restart_async() {
        if (!auto_restart_timer) {
            return;
        }
        auto_restart_timer->expires_after(std::chrono::seconds(5));
        auto_restart_timer->async_wait([this] (const boost::system::error_code &ec) {
            if (ec) {
                logger->debug("auto_restart_timer.async_wait ec={}", ec.message());
                return;
            }
            try {
                if (!(process_status == ProcessStatus::STARTED || process_status == ProcessStatus::RUNNING)) {
                    logger->info("check_and_auto_restart_async try start_ipg_process");
                    auto lock = acquire_lock();
                    start_ipg_process();
                }
            } catch (std::exception &e) {
                logger->error("check_and_auto_restart_async start_ipg_process error: {}", e.what());
            }
            check_and_auto_restart_async();
        });
    }

    void process_io() {
        auto work_guard = asio::make_work_guard(io_context); // run forever, until io_context.stop() is called
        if (enable_auto_restart) {
            auto_restart_timer.reset(new asio::steady_timer(io_context));
            check_and_auto_restart_async();
        }
        io_context.run();
    }

    void start_cmd_thread() {
        cmd_thread = std::thread([this] {
            logger->info("cmd_thread start");
            auto work_guard = asio::make_work_guard(cmd_io_context);
            cmd_io_context.run();
            logger->info("cmd_thread finish");
        });
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

    void get_lan_interfaces(const Request &req, Response &res) {
        Json::Value config;
        {
            auto lock = acquire_lock();
            config = config_json["lan_interfaces"];
        }
        json_ok(res, config);
    }

    static bool is_cidr_ip_addr(const std::string &s) {
        /* xxx.xxx.xxx.xxx/xx
        ok:
            192.168.123.209/24
            192.168.123.238/32
        err:
            192.168.123.209
            192.168.123
            asdf/23
        */
        std::vector<std::string> vec;
        boost::split(vec, s, boost::is_any_of("/"));
        if (vec.size() != 2) {
            return false;
        }
        std::string addr = vec[0];
        try {
            int suffix = boost::lexical_cast<int>(vec[1]);
            if (suffix < 0 || suffix > 32) {
                return false;
            }
        } catch (std::exception &e) {
            return false;
        }
        boost::system::error_code ec;
        asio::ip::make_address_v4(addr, ec);
        if (ec) {
            return false;
        }
        return true;
    }

    void update_lan_interfaces_addr(const Request &req, Response &res) {
        auto json_body = parse_json_body(req);

        if (!json_body.isArray()) {
            json_err(res, "require array");
            return;
        }
        std::unordered_set<std::string> used_ip_addr;
        for (Json::Value::ArrayIndex i = 0; i < json_body.size(); ++i) {
            auto &p = json_body[i];
            if (!p.isObject()) {
                json_err(res, "require object");
                return;
            }
            auto ip_addr = p["ip_address"].asString();
            if (!ip_addr.empty() && !is_cidr_ip_addr(ip_addr)) {
                json_err(res, "invalid ip address: " + ip_addr);
                return;
            }
            if (ip_addr.empty()) {
                p["ip_address"] = "0.0.0.0/0";
            } else {
                auto p = ip_addr.find("/");
                if (p == std::string::npos) {
                    json_err(res, "invalid ip address: " + ip_addr);
                    return;
                }
                std::string left_addr = ip_addr.substr(0, p);
                auto it = used_ip_addr.find(left_addr);
                if (it != used_ip_addr.end()) {
                    json_err(res, "IP Address Conflict");
                    return;
                }
                used_ip_addr.insert(left_addr);
            }
        }

        auto lock = acquire_lock();
        Json::Value lan_interfaces = config_json["lan_interfaces"];

        for (Json::Value::ArrayIndex i = 0; i < json_body.size(); ++i) {
            auto &p = json_body[i];
            auto connection_id = p["connection_id"].asString();
            int inf_index = -1;
            for (Json::Value::ArrayIndex j = 0; j < lan_interfaces.size(); ++j) {
                if (lan_interfaces[j]["connection_id"] == connection_id) {
                    inf_index = j;
                    break;
                }
            }
            if (inf_index < 0) {
                continue;
            }

            auto &inf = lan_interfaces[inf_index];
            
            auto ip_addr = p["ip_address"].asString();

            std::string method = "manual";
            if (p["method"].isString()) {
                method = p["method"].asString();
            }

            if (method == inf["method"].asString() && ip_addr == inf["ip_address"].asString()) {
                continue; // no modified
            }

            std::vector<std::string> args {
                "connection", "modify", connection_id,
                "connection.autoconnect", "true",
                "ipv4.method", method
            };

            if (method == "manual") {
                if (ip_addr.empty()) {
                    continue;
                }
                args.push_back("ipv4.addresses");
                args.push_back(ip_addr);
            }

            try {
                exec_cmd(nmcli_path, args);
            } catch (std::exception &e) {
                logger->error("update_lan_interfaces_addr exec_cmd exception: {}", e.what());
                continue;
            }
            logger->debug("update_lan_interfaces_addr: nmcli connection modify {} ipv4.addresses {}", connection_id, ip_addr);

            std::vector<std::string> up_args {
                "connection", "up", connection_id
            };
            try {
                exec_cmd(nmcli_path, up_args);
            } catch (std::exception &e) {
                logger->error("update_lan_interfaces_addr exec_cmd exception: {}", e.what());
                continue;
            }
        }

        nmcli_load_connection_info(config_json["lan_interfaces"]);

        json_ok(res);
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

    void poweroff(const Request &req, Response &res) {
        auto lock = acquire_lock();
        logger->info("poweroff by user");
        stop_ipg_process();
        json_ok(res);
        bp::child process(poweroff_path, cmd_io_context);
        process.detach();
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
                    logger->trace("start_ipg_health_check_thread client.Get(): {} {}", url, httplib::to_string(res.error()));
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
        cmd_io_context.stop();
        logger->info("stopped");
    }

    static std::vector<std::string> split_lines(const std::string &s) {
        std::istringstream iss(s);
        std::string part;
        std::vector<std::string> result;
        while (std::getline(iss, part)) {
            result.push_back(std::move(part));
        }
        return result;
    }

    static std::string join(const std::vector<std::string> &v, const std::string &sep) {
        std::ostringstream oss;
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (i > 0) {
                oss << sep;
            }
            oss << v[i];
        }
        return oss.str();
    }

    void load_lan_interfaces_info() {
        Json::Value lan_interfaces;
        {
            auto lock = acquire_lock();
            lan_interfaces = config_json["lan_interfaces"];
        }
        nmcli_load_connection_info(lan_interfaces);
        {
            auto lock = acquire_lock();
            config_json["lan_interfaces"] = lan_interfaces;
        }
    }

    void nmcli_load_connection_info(Json::Value &interfaces) {
        if (!interfaces.isArray()) {
            return;
        }
        std::vector<std::string> props {
            "ipv4.method",
            "ipv4.addresses"
        };
        std::string props_str = join(props, ",");
        for (Json::Value::ArrayIndex i = 0; i < interfaces.size(); ++i) {
            auto &inf = interfaces[i];
            if (!inf.isObject() || !inf["connection_id"].isString()) {
                continue;
            }
            auto connection_id = inf["connection_id"].asString();
            std::vector<std::string> args {
                "-g", props_str,
                "connection", "show", connection_id
            };
            std::string std_out;
            try {
                std_out = exec_cmd(nmcli_path, args);
            } catch (std::exception &e) {
                logger->error("nmcli_get_connection_info exception: {}", e.what());
                continue;
            }
            logger->debug("nmcli_get_connection_info: nmcli -g {} connection show {} std_out={}", props_str, connection_id, std_out);
            if (std_out.empty()) {
                continue;
            }
            auto lines = split_lines(std_out);
            if (lines.size() != props.size()) {
                continue;
            }
            Json::Value prop_json;
            for (std::size_t i = 0; i < props.size(); ++i) {
                prop_json[props[i]] = lines[i];
            }

            inf["method"] = prop_json["ipv4.method"];
            inf["ip_address"] = prop_json["ipv4.addresses"];
        }
    }

    std::string exec_cmd(const fs::path &path, const std::vector<std::string> &args) {
        std::future<std::string> std_out;
        std::future<std::string> std_err;
        bp::child process(
            path, 
            args, 
            cmd_io_context, 
            bp::std_out > std_out,
            bp::std_err > std_err);
        process.wait();
        int exit_code = process.exit_code();
        if (exit_code) {
            logger->error("exec_cmd {} exit_code={} std_err={}", path.string(), exit_code, std_err.get());
        }
        return std_out.get();
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
    std::thread cmd_thread;
    asio::io_context cmd_io_context;
    Json::Value config_json;
    std::mutex mutex;
    std::atomic<ProcessStatus> process_status = ProcessStatus::NOT_RUNNING;
    std::atomic<bool> stopped = false;
    bool enable_auto_restart = false;
    httplib::Server http_server;
    std::thread http_server_thread;
    std::thread ipg_health_check_thread;
    fs::path nmcli_path;
    fs::path poweroff_path;
    std::unique_ptr<asio::steady_timer> auto_restart_timer;
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