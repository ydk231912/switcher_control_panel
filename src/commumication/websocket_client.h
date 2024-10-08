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
  int m_id;
  websocketpp::connection_hdl m_hdl;
  std::string m_status;
  std::string m_uri;
  std::string m_server;
  std::string m_error_reason;
  websocket_endpoint *m_endpoint;
};

class websocket_endpoint {
public:
  websocket_endpoint();
  ~websocket_endpoint();
  void handler_message(websocketpp::connection_hdl, client::message_ptr msg);
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

  int connect(std::string const &uri);

  void close(int id, websocketpp::close::status::value code,
             std::string reason);

  void send(int id, std::string message);

  connection_metadata::ptr get_metadata(int id) const;

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

  client m_endpoint;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
  con_list m_connection_list;
  int m_next_id;
};

} // namespace seeder