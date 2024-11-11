#pragma once

#include <cstdio>
#include <vector>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <websocketpp/common/memory.hpp>
#include <websocketpp/common/thread.hpp>

#include "../lib/json.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
namespace seeder {
  // Logger
extern std::shared_ptr<spdlog::logger> logger;

class websocket_endpoint;

class connection_metadata {
public:
  typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

  connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri,websocket_endpoint *endpoint);

  void on_open(client *c, websocketpp::connection_hdl hdl);

  void on_fail(client *c, websocketpp::connection_hdl hdl);

  void on_close(client *c, websocketpp::connection_hdl hdl);

  websocketpp::connection_hdl get_hdl() const;

  int get_id() const;
  std::string get_status() const;

private:
  int m_id;                           //id
  websocketpp::connection_hdl m_hdl;  // websocketpp表示连接的编号
  std::string m_status;               // 连接自动状态
  std::string m_uri;                  // 连接的URI
  std::string m_server;               // 服务器信息
  std::string m_error_reason;         // 错误原因
  websocket_endpoint *m_endpoint;     // 端点
};

class websocket_endpoint {
public:
  websocket_endpoint();
  ~websocket_endpoint();
  
  void panel_status_handler_message(websocketpp::connection_hdl, client::message_ptr msg);
  void config_update_handler_message(websocketpp::connection_hdl, client::message_ptr msg);

  void
  set_panel_status_handler(std::function<void(int, int)> panel_status_handler);
  void 
  set_dsk_status_handler(std::function<void(std::vector<bool>)> dsk_status_handler);
  void 
  set_transition_type_handler(std::function<void(std::string)> transition_type_handler);
  void 
  set_next_transition_handler(std::function<void(std::vector<bool>)> next_transition_handler);
  void 
  set_proxy_type_handler(std::function<void(std::vector<bool>)> proxy_type_handler);
  void 
  set_proxy_source_handler(std::function<void(std::vector<bool>)> proxy_source_handler);
  void 
  set_mode_status_handler(std::function<void(std::vector<bool>)> mode_status_handler);
  void 
  set_shift_status_handler(std::function<void(std::vector<bool>)> shift_status_handler);
  void 
  set_config_update_notify_handler(std::function<void(nlohmann::json)> config_update_notify_handler);
  void 
  set_panel_status_update_handler(std::function<void(nlohmann::json)> panel_status_update_handler);

  int connect(std::string const &uri);

  void close(int id, websocketpp::close::status::value code,
             std::string reason);

  void send_message(int id, const nlohmann::json &message);
  void send_mixer_command_manual(const nlohmann::json &is_auto);
  void send_mixer_command_manual_ratio(int progress_ratio);
  
  connection_metadata::ptr get_metadata(int id) const;

  std::string ws_panel_status_str = "";
  std::string ws_config_update_notify_str = "";

private:
  typedef std::map<int, connection_metadata::ptr> con_list;
  
  std::function<void(int, int)> panel_status_handler;
  std::function<void(std::vector<bool>)> dsk_status_handler;
  std::function<void(std::string)> transition_type_handler;
  std::function<void(std::vector<bool>)> next_transition_handler;
  std::function<void(std::vector<bool>)> proxy_type_handler;
  std::function<void(std::vector<bool>)> proxy_source_handler;
  std::function<void(std::vector<bool>)> mode_status_handler;
  std::function<void(std::vector<bool>)> shift_status_handler;
  std::function<void(nlohmann::json)> panel_status_update_handler;

  std::function<void(nlohmann::json)> config_update_notify_handler;

  client m_endpoint;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
  con_list m_connection_list;
  int m_next_id = 0;
  int new_panel_status_id = 0;
  int new_config_update = 0;
};

} // namespace seeder