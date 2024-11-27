#include "http_server.h"
#include "app_base.h"
#include "app_error_code.h"
#include "app_session.h"
#include "app_stat.h"
#include "core/util/logger.h"
#include "core/util/thread.h"
#include "httplib.h"
// #include "mtl/mtl_seeder_api.h"
#include "node.h"
#include "parse_json.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <condition_variable>
#include <core/util/fs.h>
#include <cstdio>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <json.h>
#include <json/config.h>
#include <json/forwards.h>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <thread>

#include "app_build_config.h"

using httplib::Request;
using httplib::Response;

#define SAFE_LOG(level, ...)                                                   \
  if (seeder::core::logger)                                                    \
  (seeder::core::logger)->level(__VA_ARGS__)

namespace {

namespace fs = boost::filesystem;

void write_string_to_file_directly(const std::string &file_path,
                                   const std::string &content) {
  std::fstream stream(file_path, std::ios_base::out);
  stream << content;
  stream.flush();
}

double get_cpu_usage() {
  std::ifstream stat_file("/proc/stat");
  if (!stat_file.is_open()) {
    seeder::core::logger->error("Failed to open /proc/stat");
    return 0;
  }
  std::string line;
  std::getline(stat_file, line);
  stat_file.close();

  long long user, nice, system, idle, iowait, irq, softirq, steal, guest,
      guest_nice;
  sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
         &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest,
         &guest_nice);

  long long total =
      user + nice + system + idle + iowait + irq + softirq + steal;
  double cpu_usage = ((total - idle) * 100.0) / total;
  // std::cout<<std::fixed<<std::setprecision(2)<<cpu_usage<<std::endl;
  return cpu_usage;
}

double get_mem_usage() {
  typedef struct mempacked {
    char str_memtotal[20];
    unsigned long i_memtotal;
    char str_memfree[20];
    unsigned long i_memfree;
    char str_buffers[20];
    unsigned long i_buffers;
    char str_cached[20];
    unsigned long i_cached;
    char str_swapcached[20];
    unsigned long i_swapcached;

  } mem_occupy;

  FILE *fd = NULL;
  char buff[256];
  mem_occupy mem;
  mem_occupy *m = &mem;
  fd = fopen("/proc/meminfo", "r");
  if (!fd) {
    seeder::core::logger->error("Failed to open /proc/meminfo");
    return 0;
  }
  fgets(buff, sizeof(buff), fd);
  sscanf(buff, "%s %lu ", m->str_memtotal, &m->i_memtotal);
  fgets(buff, sizeof(buff), fd);
  sscanf(buff, "%s %lu ", m->str_memfree, &m->i_memfree);
  fgets(buff, sizeof(buff), fd);
  sscanf(buff, "%s %lu ", m->str_buffers, &m->i_buffers);
  fgets(buff, sizeof(buff), fd);
  sscanf(buff, "%s %lu ", m->str_cached, &m->i_cached);
  fgets(buff, sizeof(buff), fd);
  sscanf(buff, "%s %lu ", m->str_swapcached, &m->i_swapcached);

  double mem_usage = ((m->i_memtotal - m->i_memfree) * 100.0) / m->i_memtotal;
  // std::cout<<std::fixed<<std::setprecision(2)<<mem_usage<<std::endl;
  fclose(fd);
  return mem_usage;
}

void write_string_to_file(const std::string &file_path,
                          const std::string &content) {
  if (fs::exists(file_path)) {
    boost::system::error_code ec;
    std::string new_file_path = file_path + ".new";
    write_string_to_file_directly(new_file_path, content);
    std::string old_file_path = file_path + ".old";
    fs::rename(file_path, old_file_path, ec);
    if (ec) {
      SAFE_LOG(warn, "failed to mv {} {}", file_path, old_file_path,
               ec.message());
      return;
    }
    fs::rename(new_file_path, file_path, ec);
    if (ec) {
      SAFE_LOG(warn, "failed to mv {} {} : {}", new_file_path, file_path,
               ec.message());
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

class app_status_monitor {
public:
  st_app_context *app_ctx = nullptr;
  Json::Value status_value;
  std::shared_ptr<spdlog::logger> logger;
  bool is_stopped = false;
  std::mutex monitor_mutex;
  std::condition_variable stop_cv;
  std::thread monitor_thread;
  std::chrono::system_clock::duration wait_duration = std::chrono::seconds(1);

  std::unique_lock<std::mutex> acquire_ctx_lock() {
    return std::unique_lock<std::mutex>(app_ctx->mutex);
  }

  void start() {
    if (is_stopped) {
      return;
    }
    logger = seeder::core::logger;
    monitor_thread = std::thread([this] {
      logger->debug("app_status_monitor thread start");
      while (!is_stopped) {
        std::unique_lock<std::mutex> lock(monitor_mutex);
        {
          auto app_lock = acquire_ctx_lock();
          update_status();
        }
        stop_cv.wait_for(lock, wait_duration);
      }
      logger->debug("app_status_monitor thread finish");
    });
  }

  void update_status() { status_value = st_app_get_status(app_ctx); }

  Json::Value get_status() {
    Json::Value ret;
    {
      std::unique_lock<std::mutex> lock(monitor_mutex);
      ret = status_value;
    }
    return ret;
  }

  void stop() {
    if (is_stopped) {
      return;
    }

    {
      std::unique_lock<std::mutex> lock(monitor_mutex);
      is_stopped = true;
    }

    stop_cv.notify_all();
    monitor_thread.join();
  }

  ~app_status_monitor() { stop(); }
};

class JsonObjectDeleter {
public:
  void operator()(json_object *o) { json_object_put(o); }
};

} // namespace

namespace seeder {

class st_http_server::impl : boost::noncopyable {
  friend class st_http_server;

private:
  st_app_context *app_ctx = nullptr;
  httplib::Server server;
  std::thread server_thread;
  app_status_monitor status_monitor;
  std::shared_ptr<spdlog::logger> logger;
  std::chrono::system_clock::time_point startup_time;
  nmos_node *node;

  void get_config(const Request &req, Response &res) {
    Json::Value root;
    {
      auto lock = acquire_ctx_lock();
      root = app_ctx->json_ctx->json_root;
    }
    json_ok(res, root);
  }

  void get_health(const Request &req, Response &res) {
    Json::Value root;
    Json::Int64 startup_time_count =
        std::chrono::duration_cast<std::chrono::seconds>(
            startup_time.time_since_epoch())
            .count();
    root["startup_time"] = startup_time_count;
    Json::Int64 startup_duration_count =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - startup_time)
            .count();
    root["startup_duration"] = startup_duration_count;
    root["rev"] = PROJECT_GIT_HASH;
    
    json_ok(res, root);
  }

  void get_device_info(const Request &req, Response &res) {
    Json::Value root;

    double memory_usage = get_mem_usage();
    root["memory_usage"] = memory_usage;
    // 获取CPU的使用情况指令：top -n 1  |awk -F ',' '{print $4}' |grep id|awk -F
    // ' ' '{print $2}' sprintf(cmd,"top -n 1  |awk -F ',' '{print $4}' |grep
    // id|awk -F ' ' '{print $2}'");

    double cpu_usage = get_cpu_usage();
    root["cpu_usage"] = cpu_usage;

    json_ok(res, root);
  }

  void save_config(const Request &req, Response &res) {
    if (req.body.empty()) {
      json_err(res, "empty body");
      return;
    }
    auto lock = acquire_ctx_lock();
    write_string_to_file(app_ctx->config_file_path, req.body);
    logger->info("save config file {}", app_ctx->config_file_path);
  }

  void get_fmts(const Request &req, Response &res) {
    auto json_value = st_app_get_fmts(app_ctx->json_ctx.get());
    json_ok(res, json_value);
  }

  void add_session(const Request &req, Response &res) {
    auto lock = acquire_ctx_lock();
    std::unique_ptr<st_json_context_t> new_ctx;
    std::error_code ec{};
    ec = st_app_parse_json_add(app_ctx->json_ctx.get(), req.body, new_ctx);
    if (ec) {
      logger->warn("st_app_parse_json_add error {} {}", ec.value(),
                   ec.message());
      json_err(res, ec);
      return;
    }
    ec = st_app_add_tx_sessions(app_ctx, new_ctx.get());
    if (ec) {
      logger->warn("st_app_add_tx_sessions error {} {}", ec.value(),
                   ec.message());
      json_err(res, ec);
      return;
    }
    ec = st_app_add_rx_sessions(app_ctx, new_ctx.get());
    if (ec) {
      logger->warn("st_app_add_rx_sessions error {} {}", ec.value(),
                   ec.message());
      json_err(res, ec);
      return;
    }
    save_config_file();
    try {
      node->add();
    } catch (std::exception &e) {
      logger->error("http_server add_session nmos error: {}", e.what());
    }
    json_ok(res);
  }

  void update_session(const Request &req, Response &res) {
    auto root = parse_json_body(req);
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string s = Json::writeString(writer, root);
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
    std::error_code ec{};
    ec = st_app_parse_json_update(app_ctx->json_ctx.get(), req.body, new_ctx);
    if (ec) {
      logger->warn("st_app_parse_json_update error {} {}", ec.value(),
                   ec.message());
      json_err(res, ec);
      return;
    }
    ec = st_app_update_tx_sessions(app_ctx, new_ctx.get());
    if (ec) {
      logger->warn("st_app_update_tx_sessions error {} {}", ec.value(),
                   ec.message());
      json_err(res, ec);
      return;
    }
    ec = st_app_update_rx_sessions(app_ctx, new_ctx.get());
    if (ec) {
      logger->warn("st_app_update_rx_sessions error {} {}", ec.value(),
                   ec.message());
      json_err(res, ec);
      return;
    }
    save_config_file();
    try {
      auto &tx_sessions = root["tx_sessions"];
      if (tx_sessions.isArray()) {
        for (Json::ArrayIndex i = 0; i < tx_sessions.size(); ++i) {
          auto &s = tx_sessions[i];
          if (s["id"].isString() && !s["id"].asString().empty()) {
            node->udpate_sender(s["id"].asString());
          }
        }
      };
      auto &rx_sessions = root["rx_sessions"];
      if (rx_sessions.isArray()) {
        for (Json::ArrayIndex i = 0; i < rx_sessions.size(); ++i) {
          auto &s = rx_sessions[i];
          if (s["id"].isString() && !s["id"].asString().empty()) {
            node->update_receiver(s["id"].asString());
          }
        }
      };
    } catch (std::exception &e) {
      logger->error("http_server update_session nmos error: {}", e.what());
    }
    
    json_ok(res);
  }

  void remove_tx_session(const Request &req, Response &res) {
    auto root = parse_json_body(req);
    auto tx_source_id = root["tx_source_id"].asString();
    try {
      node->remove_sender(tx_source_id, true);
    } catch (std::exception &e) {
      logger->error("http_server remove_tx_session nmos error: {}", e.what());
    }
    
    if (tx_source_id.empty()) {
      json_err(res, "tx_source_id empty");
      return;
    }
    auto lock = acquire_ctx_lock();
    std::error_code ec{};
    ec = st_app_remove_tx_session(app_ctx, tx_source_id);
    if (ec) {
      logger->warn("st_app_remove_tx_session error {} {}", ec.value(),
                   ec.message());
      json_err(res, "st_app_remove_tx_session error");
      return;
    }
    save_config_file();

    json_ok(res);
  }

  void remove_rx_session(const Request &req, Response &res) {
    auto root = parse_json_body(req);
    auto rx_output_id = root["rx_output_id"].asString();
    try {
      node->remove_receiver(rx_output_id, true);
    } catch (std::exception &e) {
      logger->error("http_server remove_rx_session nmos error: {}", e.what());
    }
    
    if (rx_output_id.empty()) {
      json_err(res, "rx_output_id empty");
      return;
    }
    auto lock = acquire_ctx_lock();
    std::error_code ec{};
    ec = st_app_remove_rx_session(app_ctx, rx_output_id);
    if (ec) {
      logger->warn("st_app_remove_rx_session error {} {}", ec.value(),
                   ec.message());
      json_err(res, "st_app_remove_rx_session error");
      return;
    }
    save_config_file();

    json_ok(res);
  }

  void get_stat(const Request &req, Response &res) {
    Json::Value stat;
    {
      auto lock = acquire_ctx_lock();
      stat = app_ctx->stat;
    }
    json_ok(res, stat);
  }

  void get_status(const Request &req, Response &res) {
    Json::Value status = status_monitor.get_status();
    json_ok(res, status);
  }

  /* TODO dlopen 
  void get_ptp_dev_info(const Request &req, Response &res) {
    json_object *tmp_json = nullptr;
    mtl_seeder_get_ptp_dev_info(app_ctx->st, &tmp_json);
    std::unique_ptr<json_object, JsonObjectDeleter> ptp_info(tmp_json);
    json_ok(res, json_object_to_json_string(ptp_info.get()));
  } */

  void save_config_file() {
    try {
      Json::Reader reader;
      Json::Value config_json;
      reader.parse(read_file_to_string(app_ctx->config_file_path), config_json);
      // app_ctx->json_ctx->json_root["interfaces"] = config_json["interfaces"];
      config_json["tx_sessions"] = app_ctx->json_ctx->json_root["tx_sessions"];
      config_json["rx_sessions"] = app_ctx->json_ctx->json_root["rx_sessions"];
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

  static void json_ok(Response &res) {
    res.status = 200;
    res.set_content("{}", "application/json");
  }

  static void json_ok(Response &res, const char *json_content) {
    res.status = 200;
    res.set_content(json_content, "application/json");
  }

  static void json_ok(Response &res, const std::string &json_content) {
    res.status = 200;
    res.set_content(json_content, "application/json");
  }

  static void json_ok(Response &res, const Json::Value &value) {
    Json::FastWriter writer;
    std::string output = writer.write(value);
    json_ok(res, output);
  }

  static void text_err(Response &res, const std::string &message) {
    res.status = 400;
    res.set_content(message, "text/plain");
  }

  static void text_err(Response &res, const std::string &message,
                       const std::error_code &ec) {
    text_err(res, message + ": " + ec.message());
  }

  static void json_err_content(Response &res, const Json::Value &value) {
    Json::FastWriter writer;
    std::string output = writer.write(value);
    res.status = 400;
    res.set_content(output, "application/json");
  }

  static void json_err(Response &res, const std::string &message) {
    Json::Value root;
    root["error_code"] = 0;
    root["error_message"] = message;
    json_err_content(res, root);
  }

  static void json_err(Response &res, const std::error_code &ec) {
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

st_http_server::st_http_server(st_app_context *app_ctx, nmos_node *node)
    : p_impl(new impl) {
  p_impl->app_ctx = app_ctx;
  p_impl->logger = seeder::core::logger;
  p_impl->status_monitor.app_ctx = app_ctx;
  p_impl->node = node;
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

  server.set_logger([&p](const Request &req, const Response &res) {
    p.logger->debug("http {} status code {}", req.path, res.status);
  });

  server.Get("/api/json", [&p](const Request &req, Response &res) {
    p.get_config(req, res);
  });

  server.Get("/api/get_fmt",
             [&p](const Request &req, Response &res) { p.get_fmts(req, res); });

  server.Get("/api/health", [&p](const Request &req, Response &res) {
    p.get_health(req, res);
  });

  server.Post("/api/saveConfig", [&p](const Request &req, Response &res) {
    p.save_config(req, res);
  });

  server.Post("/api/add_tx_json", [&p](const Request &req, Response &res) {
    p.add_session(req, res);
  });

  server.Post("/api/update_tx_json", [&p](const Request &req, Response &res) {
    p.update_session(req, res);
  });

  server.Post("/api/remove_tx_json", [&p](const Request &req, Response &res) {
    p.remove_tx_session(req, res);
  });

  server.Post("/api/add_rx_json", [&p](const Request &req, Response &res) {
    p.add_session(req, res);
  });

  server.Post("/api/update_rx_json", [&p](const Request &req, Response &res) {
    p.update_session(req, res);
  });

  server.Post("/api/remove_rx_json", [&p](const Request &req, Response &res) {
    p.remove_rx_session(req, res);
  });

  server.Get("/api/stat",
             [&p](const Request &req, Response &res) { p.get_stat(req, res); });

  server.Get("/api/status", [&p](const Request &req, Response &res) {
    p.get_status(req, res);
  });

  /* server.Get("/api/ptp_dev_info", [&p](const Request &req, Response &res) {
    p.get_ptp_dev_info(req, res);
  }); */

  server.Get("/api/device_info", [&p](const Request &req, Response &res) {
    p.get_device_info(req, res);
  });

  p.startup_time = std::chrono::system_clock::now();

  p.server_thread = std::thread([&] {
    seeder::core::util::set_thread_name("http_server");
    const std::string host = "0.0.0.0";
    int port = p.app_ctx->http_port;
    p.logger->info("http server listen on {}:{}", host, port);
    server.listen(host, port);
    p.logger->info("http server stopped");
  });

  p.status_monitor.start();
}

void st_http_server::stop() {
  p_impl->server.stop();
  p_impl->status_monitor.stop();
}

} // namespace seeder
