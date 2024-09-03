#include "websocket_client.h"
#include <vector>
namespace seeder {

connection_metadata::connection_metadata(int id,
                                         websocketpp::connection_hdl hdl,
                                         std::string uri)
    : m_id(id), m_hdl(hdl), m_status("Connecting"), m_uri(uri),
      m_server("N/A") {}
void connection_metadata::on_open(client *c, websocketpp::connection_hdl hdl) {
  //更新连接状态
  m_status = "Open";

  //获取连接指针 
  client::connection_ptr con = c->get_con_from_hdl(hdl);

  //获取并存储响应头中的 "Server" 信息
  m_server = con->get_response_header("Server");
  //printf("Connection %d opened to %s\n", m_id, m_uri.c_str());
}

void connection_metadata::on_fail(client *c, websocketpp::connection_hdl hdl) {
  m_status = "Failed";

  client::connection_ptr con = c->get_con_from_hdl(hdl);
  m_server = con->get_response_header("Server");
  m_error_reason = con->get_ec().message(); //获取并存储错误原因
  // printf("Connection %d failed to connect with error: %s\n", m_id,
  //         m_error_reason.c_str());
}

void connection_metadata::on_close(client *c, websocketpp::connection_hdl hdl) {
  client::connection_ptr con = c->get_con_from_hdl(hdl);

  // printf("Connection %d closed with code %d (%s)\n", m_id,
  //         con->get_remote_close_code(),
  //         websocketpp::close::status::get_string(con->get_remote_close_code()));

}

websocketpp::connection_hdl connection_metadata::get_hdl() const {
  return m_hdl;
}

int connection_metadata::get_id() const { return m_id; }

std::string connection_metadata::get_status() const { return m_status; }

//初始化 WebSocket 端点的状态和设置
websocket_endpoint::websocket_endpoint() : m_next_id(0) {
  //清除日志访问和错误频道
  m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
  m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

  //初始化 Asio
  m_endpoint.init_asio();

  //启动 WebSocket 处理线程
  m_endpoint.start_perpetual();

  //创建和启动线程
  m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(
      &client::run, &m_endpoint);
};

void websocket_endpoint::handler_message(websocketpp::connection_hdl,
                                         client::message_ptr msg) {
  nlohmann::json json_data = nlohmann::json::parse(msg->get_payload());

  // 处理 PGM PVW 状态
  if (panel_status_handler) {
    int pgm_index = json_data["pgm"]["background_source_index"];
    int pvw_index = json_data["pvw"]["background_source_index"];
    panel_status_handler(pgm_index, pvw_index);
  }

  // 处理 DSK 状态
  if (dsk_status_handler) {
    std::vector<bool> dsk_status;
    nlohmann::json json_arr = json_data["downstream_keys"];
    for (const auto &element : json_arr) {
      dsk_status.push_back(element["is_on_air"]);
    };
    dsk_status_handler(dsk_status);
  }

  // 处理 过度效果类型 状态
  if (transition_type_handler) {
    std::string activate_transition_type =
        json_data["transition_status"]["active_type"];
    transition_type_handler(activate_transition_type);
  }

  // 处理 下一个过度效果类型 状态
  if (next_transition_handler) {
    std::vector<bool> next_transition_type =
        json_data["next_transition_status"]["next_type"];
    next_transition_handler(next_transition_type);
  }

  // 代理键
  if (proxy_type_handler) {
    std::vector<bool> proxy_type =
        json_data["proxy_type"]["proxy_type"];
    proxy_type_handler(proxy_type);
  }

  // 代理源
  if (proxy_source_handler) {
    std::vector<bool> proxy_source =
        json_data["proxy_source"]["proxy_type_source"];
    proxy_source_handler(proxy_source);
  }

  //mode
  if (mode_status_handler) {
    std::vector<bool> mdoe_status =
        json_data["mode_status"]["mode_type"];
    mode_status_handler(mdoe_status);
  }

  // keytrans
  if (shift_status_handler) {
    std::vector<bool> shift_status =
        json_data["shift_status"]["shift_type"];
    shift_status_handler(shift_status);
  }
  
}

void websocket_endpoint::set_panel_status_handler(
    std::function<void(int, int)> panel_status_handler) {
  this->panel_status_handler = panel_status_handler;
}

void websocket_endpoint::set_dsk_status_handler(
    std::function<void(std::vector<bool>)> dsk_status_handler) {
  this->dsk_status_handler = dsk_status_handler;
}

void websocket_endpoint::set_transition_type_handler(
    std::function<void(std::string)> transition_type_handler) {
  this->transition_type_handler = transition_type_handler;
}

void websocket_endpoint::set_next_transition_handler(
    std::function<void(std::vector<bool>)> next_transition_handler) {
  this->next_transition_handler = next_transition_handler;
}

void websocket_endpoint::set_proxy_type_handler(
    std::function<void(std::vector<bool>)> proxy_type_handler) {
  this->proxy_type_handler = proxy_type_handler;
}

void websocket_endpoint::set_proxy_source_handler(
    std::function<void(std::vector<bool>)> proxy_source_handler) {
  this->proxy_source_handler = proxy_source_handler;
}

void websocket_endpoint::set_mode_status_handler(
    std::function<void(std::vector<bool>)> mode_status_handler) {
  this->mode_status_handler = mode_status_handler;
}

void websocket_endpoint::set_shift_status_handler(
    std::function<void(std::vector<bool>)> shift_status_handler) {
  this->shift_status_handler = shift_status_handler;
}


websocket_endpoint::~websocket_endpoint() {
  //停止 WebSocket 处理
  m_endpoint.stop_perpetual();

  //关闭所有打开的连接
  for (con_list::const_iterator it = m_connection_list.begin();
       it != m_connection_list.end(); ++it) {
    if (it->second->get_status() != "Open") {
      // Only close open connections
      continue;
    }
    std::cout << "> Closing connection " << it->second->get_id() << std::endl;

    websocketpp::lib::error_code ec;
    m_endpoint.close(it->second->get_hdl(),
                     websocketpp::close::status::going_away, "", ec);
    if (ec) {
      std::cout << "> Error closing connection " << it->second->get_id() << ": "
                << ec.message() << std::endl;
    }
  }

  m_thread->join();
}

//连接到 WebSocket 服务器
int websocket_endpoint::connect(std::string const &uri) {
  sleep(5);
  //创建连接 -- uri是WebSocket 服务器的 URI
  websocketpp::lib::error_code ec;
  client::connection_ptr con = m_endpoint.get_connection(uri, ec);

  if (ec) {
    // printf("Failed to connect to %s\n", uri.c_str());
    logger->error("Failed to connect to {} with error: {}", uri, ec.message());
    sleep(1);
    this->connect(uri);
    return -1;
  }

  //创建连接元数据
  int new_id = m_next_id++;
  connection_metadata::ptr metadata_ptr =
      websocketpp::lib::make_shared<connection_metadata>(
          new_id, con->get_handle(), uri);
  m_connection_list[new_id] = metadata_ptr;

  // 设置连接事件处理器
  con->set_open_handler(
      websocketpp::lib::bind(&connection_metadata::on_open, metadata_ptr,
                             &m_endpoint, websocketpp::lib::placeholders::_1));
  con->set_fail_handler(
      [this, uri](websocketpp::connection_hdl hdl) { this->connect(uri); });
  con->set_close_handler(
      websocketpp::lib::bind(&connection_metadata::on_close, metadata_ptr,
                             &m_endpoint, websocketpp::lib::placeholders::_1));
  con->set_message_handler(
      websocketpp::lib::bind(&websocket_endpoint::handler_message, this,
                              websocketpp::lib::placeholders::_1, 
                              websocketpp::lib::placeholders::_2));

  //启动连接
  m_endpoint.connect(con);

  return new_id;
}

//关闭WebSocket 连接
void websocket_endpoint::close(int id, websocketpp::close::status::value code,
                               std::string reason) {
  websocketpp::lib::error_code ec;

  //查找连接
  con_list::iterator metadata_it = m_connection_list.find(id);
  if (metadata_it == m_connection_list.end()) {
    std::cout << "> No connection found with id " << id << std::endl;
    return;
  }

  //发起关闭操作
  m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
  if (ec) {
    std::cout << "> Error initiating close: " << ec.message() << std::endl;
  }
}

//通过 WebSocket 发送文本消息
void websocket_endpoint::send(int id, std::string message) {
  websocketpp::lib::error_code ec;

  //查找连接元数据
  con_list::iterator metadata_it = m_connection_list.find(id);
  if (metadata_it == m_connection_list.end()) {
    std::cout << "> No connection found with id " << id << std::endl;
    return;
  }

  //发送消息
  m_endpoint.send(metadata_it->second->get_hdl(), message,
                  websocketpp::frame::opcode::text, ec);
  if (ec) {
    std::cout << "> Error sending message: " << ec.message() << std::endl;
    return;
  }
}

//根据连接 ID 从连接列表中检索连接的元数据
connection_metadata::ptr websocket_endpoint::get_metadata(int id) const {
  //查找连接元数据
  con_list::const_iterator metadata_it = m_connection_list.find(id);

  //检查连接是否存在
  if (metadata_it == m_connection_list.end()) {
    return connection_metadata::ptr();
  } else {
    return metadata_it->second;
  }
}
} // namespace seeder