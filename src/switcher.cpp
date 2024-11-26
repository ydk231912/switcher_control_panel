#include "switcher.h"
#include <cstdio>
#include <fstream>
#include <string>
#include "commumication/ascll_convert_16_demo.h"
#include "commumication/uart_send.h"

namespace seeder {

std::shared_ptr<seeder::ASCIIConvert16> ascll_convert_16;

Switcher::Switcher(std::string config_file_path)
    : config_file(config_file_path) {}

Switcher::~Switcher() {
  stop();
}

bool Switcher::init() {
  if (config_file.empty()) {
    logger->error("config file failed to configure");
  }

  control_panel_config.reset(new config::Config);

  //读取配置文件
  std::ifstream file(config_file);
  config = nlohmann::json::parse(file);

  loadConfig(config);

  // 初始化 HTTP 客户端
  http_client = std::make_shared<HttpClient>(control_panel_config->url_config.http_url);
  http_client->cli_->set_connection_timeout(0,control_panel_config->http_timeout.http_connection_timeout);
  http_client->cli_->set_write_timeout(0,control_panel_config->http_timeout.http_write_timeout);
  http_client->cli_->set_read_timeout(0,control_panel_config->http_timeout.http_read_timeout);

  http_service = std::make_shared<HttpServer>("192.168.123.145", 12001);
  http_service->start();

  // 初始化 WebSocket 端点
  endpoint_panel_status = std::make_shared<seeder::websocket_endpoint>();
  endpoint_config_update = std::make_shared<seeder::websocket_endpoint>();
  
  endpoint_panel_status->ws_panel_status_str = control_panel_config->url_config.ws_url_panel_status;
  endpoint_config_update->ws_config_update_notify_str = control_panel_config->url_config.ws_url_config_update;

  endpoint_panel_status->connect(control_panel_config->url_config.ws_url_panel_status);
  endpoint_config_update->connect(control_panel_config->url_config.ws_url_config_update);

  return true;
}

void Switcher::stop() {
  http_service->stop();
}

void Switcher::loadConfig(nlohmann::json &config) {
  //提取配置参数
  auto &url_config = config["url_config"];
  control_panel_config->url_config.http_url = url_config["http_url"];
  control_panel_config->url_config.ws_url_panel_status = url_config["ws_url_panel_status"];
  control_panel_config->url_config.ws_url_config_update = url_config["ws_url_config_update"];
  control_panel_config->url_config.screen_path = url_config["screen_path"];
  auto &auto_config = config["auto_config"];
  control_panel_config->auto_config.auto_transition_frame_num = auto_config["auto_transition_frame_num"];
  control_panel_config->auto_config.auto_transition_sec_num = auto_config["auto_transition_sec_num"];
  auto &http_timeout = config["http_timeout"];
  control_panel_config->http_timeout.http_connection_timeout = http_timeout["http_connection_timeout"];
  control_panel_config->http_timeout.http_read_timeout = http_timeout["http_read_timeout"];
  control_panel_config->http_timeout.http_write_timeout = http_timeout["http_write_timeout"];
  
  auto &j_keyboard_config = config["switcher_keyboard_type"]["switcher_keyboard_config"];
  control_panel_config->panel_config.screen_data_size = j_keyboard_config["screen_data_size"];
  control_panel_config->panel_config.num_screen = j_keyboard_config["num_screen"];
  control_panel_config->panel_config.num_slave = j_keyboard_config["num_slave"];
  control_panel_config->panel_config.oled_frame_size = j_keyboard_config["oled_frame_size"];
  control_panel_config->panel_config.main_ctrl_keys_size = j_keyboard_config["main_ctrl_keys_size"];
  control_panel_config->panel_config.slave_keys_size = j_keyboard_config["slave_keys_size"];
  control_panel_config->panel_config.key_frame_size = j_keyboard_config["key_frame_size"];
  control_panel_config->panel_config.key_each_row_num = j_keyboard_config["key_each_row_num"];
  control_panel_config->panel_config.key_difference_value = j_keyboard_config["key_difference_value"];
  control_panel_config->panel_config.slave_key_each_row_num = j_keyboard_config["slave_key_each_row_num"];
  control_panel_config->panel_config.max_source_num = j_keyboard_config["max_source_num"];
  control_panel_config->panel_config.switcher_keyboard_id = config["switcher_keyboard_type"]["switcher_keyboard_id"];

  auto &uart_config = config["switcher_keyboard_type"]["uart_config"];
  control_panel_config->panel_config.uart_port1 = uart_config["uart_port"]["port1"];
  control_panel_config->panel_config.uart_port2 = uart_config["uart_port"]["port2"];
  control_panel_config->panel_config.uart_baudrate = uart_config["uart_baudrate"];

  auto &can_config = config["switcher_keyboard_type"]["can_config"];
  control_panel_config->panel_config.can_port = can_config["can_port"];
  control_panel_config->panel_config.can_frame_ids.push_back(can_config["fader_board_can_frame_id"]);
  control_panel_config->panel_config.can_frame_ids.push_back(can_config["one_display_board_can_frame_id"]);
  control_panel_config->panel_config.can_frame_ids.push_back(can_config["two_display_board_can_frame_id"]);
  if(control_panel_config->panel_config.switcher_keyboard_id == 2){
    control_panel_config->panel_config.can_frame_ids.push_back(can_config["three_display_board_can_frame_id"]);
    control_panel_config->panel_config.can_frame_ids.push_back(can_config["four_display_board_can_frame_id"]);
    control_panel_config->panel_config.can_frame_ids.push_back(can_config["five_display_board_can_frame_id"]);
    control_panel_config->panel_config.can_frame_ids.push_back(can_config["six_display_board_can_frame_id"]);
  }
}

nlohmann::json Switcher::get_screens_config(const std::string& screen_path){
  nlohmann::json screens_config;
  screens_config = http_client->get_json(screen_path);
  return screens_config;
}

static void take_source_xpt_commands(const nlohmann::json &sources,nlohmann::json &out_commands) {
  out_commands = nlohmann::json::array();
  if (!sources.is_array()) {return;}
  for (auto &source : sources) {
    nlohmann::json command;
    auto &j_commands = source.at("commands");
    if (j_commands.is_array()) {
      for (auto &j_command : j_commands) {
        if (j_command.at("id") == "xpt") {
          command = j_command;
          break;
        }
      }
    }
    out_commands.push_back(command);
  }
}

static void take_downstream_keys_commands(const nlohmann::json &proxy_keys, nlohmann::json &out_commands) {
  out_commands = nlohmann::json::array();
  if (!proxy_keys.is_array()) {return;}
  for (auto &key : proxy_keys) {
    nlohmann::json on_air_command;
    nlohmann::json tie_command;
    // 创建新的commands数组
    nlohmann::json new_commands;
    auto &j_commands = key.at("commands");
    if (j_commands.is_array()) {
      for (auto &j_command : j_commands) {
        if (j_command.at("id") == "on_air") {
          on_air_command = j_command;
        }
        if (j_command.at("id") == "tie") {
          tie_command = j_command;
        }
        new_commands = nlohmann::json::array();
        if (!on_air_command.empty()) {
          new_commands.push_back(on_air_command);
        }
        if (!tie_command.empty()) {
          new_commands.push_back(tie_command);
        }
        break;
      }
    }
    out_commands.push_back(new_commands);
  }
}

void Switcher::store_screens_config(
  std::vector<std::vector<std::vector<std::string>>> &screen_data,
  nlohmann::json &pgm_commands, nlohmann::json &pvw_commands, nlohmann::json &proxy_keys_commands
) {
  nlohmann::json config_screen = get_screens_config(control_panel_config->url_config.screen_path);
  auto &panel_config = control_panel_config->panel_config;
  auto &pgm_name = control_panel_config->screen_config.pgm_name;
  auto &pvw_name = control_panel_config->screen_config.pvw_name;
  auto &proxy_name = control_panel_config->screen_config.proxy_name;
  auto &proxy_sources_name = control_panel_config->screen_config.proxy_sources_name;
  auto &key_each_row_num = panel_config.key_each_row_num;
  int panel_config_num = key_each_row_num - 1;

  pgm_name.clear();
  pvw_name.clear();
  proxy_name.clear();
  proxy_sources_name.clear();
  
  for(auto &source : config_screen["pgm"]["sources"]){
    pgm_name.push_back(source["name"]);
  }
  for(auto &source : config_screen["pvw"]["sources"]){
    pvw_name.push_back(source["name"]);
  }
  //代理键第一行 key name
  for(auto &source : config_screen["downstream_keys"]){
    proxy_name.push_back(source["name"]);
  }
  //代理键第二行 key sources name 
  for(auto &source : config_screen["delegate_key_sources"]){
    proxy_sources_name.push_back(source["name"]);
  }

  std::vector<std::vector<std::vector<std::string>>> hexVector(
      4, std::vector<std::vector<std::string>>(key_each_row_num, std::vector<std::string>(5)));
  if(!pgm_name.empty()) {
    this->pgm_source_sum = pgm_name.size();
    get_hex_vector(pgm_name, hexVector[0], this->pgm_shift_status, panel_config_num);//pgm
  } else {
    logger->warn("pgm input source received null");
  }
  if(!pvw_name.empty()) {
    this->pvw_source_sum = pvw_name.size();
    get_hex_vector(pvw_name, hexVector[1], this->pvw_shift_status, panel_config_num);//pvw
  } else {
    logger->warn("pvw input source received null");
  }
  //代理键第一行 key name
  if(!proxy_name.empty()) {
    this->proxy_sum = proxy_name.size();
    get_hex_vector(proxy_name, hexVector[2],-1, panel_config_num);//代理键
  } else {
    logger->warn("key type input source received null");
  }
  //代理键第二行 key sources name 
  if(!proxy_sources_name.empty()) {
    this->proxy_sources_sum = proxy_sources_name.size();
    get_hex_vector(proxy_sources_name, hexVector[3], this->proxy_source_shift_status, panel_config_num);//代理键源
  } else {
      logger->warn("key input source received null");
  }

  std::vector<std::vector<std::vector<std::string>>> data(
      panel_config.num_slave, std::vector<std::vector<std::string>>(panel_config.num_screen, std::vector<std::string>(panel_config.screen_data_size, "")));
  update_oled_display(data, hexVector, panel_config.switcher_keyboard_id);

  screen_data = data;

  auto &pgm_sources = config_screen["pgm"]["sources"];
  auto &pvw_sources = config_screen["pvw"]["sources"];
  auto &proxy_keys = config_screen["downstream_keys"];
  auto &proxy_key_sources = config_screen["delegate_key_sources"];
  take_source_xpt_commands(pgm_sources, pgm_commands);
  take_source_xpt_commands(pvw_sources, pvw_commands);
  take_downstream_keys_commands(proxy_keys, proxy_keys_commands);
  // take_delegate_key_sources_commands(delegate_key_sources, delegate_key_sources_commands);
}

void Switcher::get_hex_vector(
  const std::vector<std::string>& names, 
  std::vector<std::vector<std::string>>& hexVector, 
  int shift_status, int panel_config_num
) {
  if (shift_status < 0) {
    int surrogate_key_type = ((-1)*shift_status)-1;
    for (int i = 0; i < panel_config_num && i < names.size(); ++i) {
      hexVector[i] = ascll_convert_16->asciiToHex(names[i]);
    }
    hexVector[panel_config_num] = ascll_convert_16->asciiToHex(surrogate_key_type_name[surrogate_key_type]);
  }
  if (shift_status >= 0) {
    int source_offset = shift_status * panel_config_num;
    for (int i = 0; i < panel_config_num && i + source_offset < names.size(); ++i) {
      hexVector[i] = ascll_convert_16->asciiToHex(names[i + source_offset]);
    }
    hexVector[panel_config_num] = ascll_convert_16->asciiToHex(shift_name[shift_status]);
  }
    
}

void Switcher::update_oled_display(
  std::vector<std::vector<std::vector<std::string>>>& data, 
  const std::vector<std::vector<std::vector<std::string>>>& hexVector, int switcher_keyboard_id
) {
  const int BUTTONS_PER_ROW = 4;
  const int BYTES_PER_BUTTON = 5;
  auto &slave_key_each_row_num = control_panel_config->panel_config.slave_key_each_row_num;
  for (size_t section = 0; section < 3; ++section) {
    for (size_t row = 0; row < 2; ++row) {
      for (size_t col = 0; col < 4; ++col) {
        for (size_t depthIdx = 0; depthIdx < 5; ++depthIdx) {
          size_t dataIndex = row * BUTTONS_PER_ROW * BYTES_PER_BUTTON + col * BYTES_PER_BUTTON + depthIdx;
          size_t displayCol = section * BUTTONS_PER_ROW + col;
          // 根据键盘 ID 更新数据
          if (switcher_keyboard_id == 1) {
              data[0][section][dataIndex] = hexVector[row+2][displayCol][depthIdx];
              // data[0][section][dataIndex] = hexVector[row][displayCol][depthIdx];
              data[1][section][dataIndex] = hexVector[row][displayCol][depthIdx];
          } else if (switcher_keyboard_id == 2) {
            for (int i = 0; i < 3; ++i) {
              data[i][section][dataIndex] = hexVector[row+2][displayCol + i * slave_key_each_row_num][depthIdx];
              // data[i][section][dataIndex] = hexVector[row][displayCol + i * slave_key_each_row_num][depthIdx];
              data[i+3][section][dataIndex] = hexVector[row][displayCol + i * slave_key_each_row_num][depthIdx];
            }
          }
        }
      }
    }
  }
}

void Switcher::execute_xpt_command(const nlohmann::json &command) {
  http_client->post("/api/panel/mixer_command", command.dump());
}
void Switcher::execute_on_air_command(const nlohmann::json &command) {
  http_client->post("/api/panel/mixer_command", command.dump());
}
//pgm pvw ket fill
void Switcher::xpt_pgm(int index) {
  if (!pgm_commands.is_array()) {
    return;
  }
  if (index < 0 || index >= pgm_commands.size()) {
    return;
  }
  execute_xpt_command(pgm_commands.at(index));
}
void Switcher::xpt_pvw(int index) {
  if (!pvw_commands.is_array()) {
    return;
  }
  if (index < 0 || index >= pvw_commands.size()) {
    return;
  }
  execute_xpt_command(pvw_commands.at(index));
}
// void Switcher::on_air_tie_fill(int index) {
//   if (!key_fill_commands.is_array()) {
//     return;
//   }
//   if (index < 0 || index >= key_fill_commands.size()) {
//     return;
//   }
//   execute_on_air_command(key_fill_commands.at(index));
// }
  
//cut
void Switcher::cut() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "StartTransition",
    "sec_num": 0,
    "frame_num": 0
}
)");
  logger->info("cut transition");
  http_client->post("/api/panel/mixer_command", command.dump());
}

//Set Transition
void Switcher::set_transition(const nlohmann::json &type) {
  nlohmann::json command {
    {"type_name", "SetTransition"},
  };
  if (type.is_string()) {
    command["transition"] = {
      {"type", type}
    };
  }
  http_client->post("/api/panel/mixer_command", command.dump());
}

//推子手动执行切换
void Switcher::mixer_command_manual(const nlohmann::json &is_auto) {
  endpoint_panel_status->send_mixer_command_manual(is_auto);
}
//推子手动执行切换进度
void Switcher::mixer_command_manual_ratio(int progress_ratio) {
  endpoint_panel_status->send_mixer_command_manual_ratio(progress_ratio);
}

//prevtrans
void Switcher::prevtrans() {
  nlohmann::json command = nlohmann::json::parse(R"(
{
    "type_name": "SetTransition",
    "transition": {
        "type": "prevtrans"
    }
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}

//bkgd -- on_air
void Switcher::bkgd() {
  nlohmann::json command = nlohmann::json::parse(R"(
{
    "id": "on_air",
    "type_name": "SetTransition",
    "transition": {
        "type": "bkgd"
    }
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}

//dsk
void Switcher::dsk(int dsk_index) {
  nlohmann::json command = nlohmann::json::parse(R"(
{
    "id": "on_air",
    "key_index": 0,
    "property": "OnAir",
    "target": "DSK",
    "type_name": "KeyToggle"
}
)");
  command["key_index"] = dsk_index;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//nextkey
void Switcher::nextkey(int nextkey_index) {
  nlohmann::json command = nlohmann::json::parse(R"(
{
    "id": "on_air",
    "key_index": 0,
    "property": "OnAir",
    "target": "NEXTKEY",
    "type_name": "SetNextTransition"
}
)");
  command["key_index"] = nextkey_index;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//auto
void Switcher::auto_() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "StartTransition",
    "sec_num": 0,
    "frame_num": 20
}
)");
  command["frame_num"] = control_panel_config->auto_config.auto_transition_frame_num;
  command["sec_num"] = control_panel_config->auto_config.auto_transition_sec_num;
  logger->info("auto transition");
  http_client->post("/api/panel/mixer_command", command.dump());
}

// //key
// void Switcher::proxy_key(int idx) {
//   this->proxy_key = idx;
// }

//1 MODE按键 -- 位于代理键的上行最后一个按键 -- 按一次切换到：KEY->AUX->UTL->MACRO->SNAPSHOT->M/E，长按可以对KEY、AUX、UTL、MACRO、SNAPSHOT、M/E进行选择
void Switcher::mode() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "ProxySelection",
    "target": "MODE",
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}

void Switcher::proxy(const std::string& source) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "Proxy",
    "target": "SOURCE",
}
)");
  command["target"] = source;
  http_client->post("/api/panel/mixer_command", command.dump());
}

void Switcher::proxy_sources(int key_source_index , const std::string& source) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_proxy_source",
    "type_name": "ProxySource",
    "target": "SOURCE",
    "source_index": 0,
    "property": "OnSource",
}
)");
  command["source_index"] = key_source_index;
  command["target"] = source;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.1 MODE/KEY按键 -- 位于再上行第一个按键 
void Switcher::proxy_key() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "ProxyKey",
    "target": "KEY",
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.1.1 KEY1-8 -- 位于代理键的第1-8个按键
void Switcher::proxy_key_source(int key_source_index) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_key_source",
    "type_name": "ProxyKEYSource",
    "target": "KEYSOURCE",
    "source_index": 0,
    "property": "OnSource",
}
)");
  command["source_index"] = key_source_index;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.1.2 CAM1-6	VS1-4	CG1-2	VR1-2	AR1-2	FM1-2	COL1-3	MP1-2	BLK	1nd/2nd 每个KEY都对应这些代理源
void Switcher::proxy_source(int source_index , const std::string& source) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_source",
    "type_name": "ProxySource",
    "target": "SOURCE",
    "source_index": 0,
    "property": "OnSource",
}
)");
  command["source_index"] = source_index;
  command["target"] = source;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.2 MODE/AUX按键 -- 位于再上行第二个按键
void Switcher::proxy_aux() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "ProxyAUX",
    "target": "AUX",
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.2.1 AUX1-8 -- 位于代理键的第1-8个按键
void Switcher::proxy_aux_source(int aux_source_index) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_aux_source",
    "type_name": "ProxyAUXSource",
    "target": "AUXSOURCE",
    "source_index": 0,
    "property": "OnSource",
}
)");
  command["source_index"] = aux_source_index;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.2.2 CAM1-6	VS1-4	CG1-2	VR1-2	AR1-2	FM1-2	COL1-3	MP1-2	BLK	1nd/2nd 每个AUX都对应这些代理源

//1.3 MODE/UTL按键 -- 位于再上行第三个按键
void Switcher::proxy_utl() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "ProxyUtl",
    "target": "UTL",
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}
void Switcher::proxy_utl_source(int utl_source_index) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_utl_source",
    "type_name": "ProxyUtlSource",
    "target": "UTLSOURCE",
    "source_index": 0,
    "property": "OnSource",
}
)");
  command["source_index"] = utl_source_index;
  http_client->post("/api/panel/mixer_command", command.dump());
}
//1.4 MODE/MACRO按键 -- 位于再上行第四个按键
void Switcher::proxy_macro() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "ProxyMacro",
    "target": "MACRO",
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.4.1 MCR1-10 -- 位于代理键的第1-10个按键
void Switcher::proxy_macro_source(int macro_source_index) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_macro_source",
    "type_name": "ProxyMACROSource",
    "target": "MACROSOURCE",
    "source_index": 0,
    "property": "OnSource",
}
)");
  command["source_index"] = macro_source_index;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.4.2 VS1-4	CG1-2	VR1-2	AR1-2	FM1-2	COL1-3	MP1-2	BLK	1nd/2nd 每个MCR都对应这些代理源

//1.5 MODE/SNAPSHOT按键 -- 位于再上行第五个按键
void Switcher::proxy_snapshot() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "ProxySnapshot",
    "target": "SNAPSHOT",
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.5.1 SNSH1-10 -- 位于代理键的第1-10个按键
void Switcher::proxy_snapshot_source(int snapshot_source_index) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_snapshot_source",
    "type_name": "ProxySNAPSHOTSource",
    "target": "SNAPSHOTSOURCE",
    "source_index": 0,
    "property": "OnSource",
}
)");
  command["source_index"] = snapshot_source_index;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.5.2 VS1-4	CG1-2	VR1-2	AR1-2	FM1-2	COL1-3	MP1-2	BLK	1nd/2nd 每个SNSH都对应这些代理源

//1.6 MODE/M/E按键 -- 位于再上行第六个按键
void Switcher::proxy_m_e() {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "type_name": "ProxyM_E",
    "target": "M_E",
}
)");
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.6.1 M/E、2M/E、3M/E、....  -- 位于代理键的第1-3...个按键
void Switcher::proxy_m_e_source(int m_e_source_index) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_m_e_source",
    "type_name": "ProxySM_ESource",
    "target": "M_ESOURCE",
    "source_index": 0,
    "property": "OnSource",
}
)");
  command["source_index"] = m_e_source_index;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//1.6.2 代理源为空

//2 SHIFT按键 -- 区分PGM源、PVW源、代理源 -- 当前源大于23用于翻页 -- 1nd/2nd/3nd/...
void Switcher::shift(const std::string& source) {
  nlohmann::json command = nlohmann::json::parse(R"(
  {
    "id": "on_shift",
    "type_name": "ShiftSource",
    "target": "PGM",
    "property": "OnShift",
}
)");
  command["target"] = source;
  http_client->post("/api/panel/mixer_command", command.dump());
}

//2.1 PGM源SHIFT按键 

//2.2 PVW源SHIFT按键

//2.3 代理源SHIFT按键

void Switcher::set_panel_status_handler(
    std::function<void(int, int)> panel_status_handler) {
  endpoint_panel_status->set_panel_status_handler(panel_status_handler);
}

void Switcher::set_dsk_status_handler(
    std::function<void(std::vector<bool>)> dsk_status_handler) {
  endpoint_panel_status->set_dsk_status_handler(dsk_status_handler);
}

void Switcher::set_transition_type_handler(
    std::function<void(std::string)> transition_type_handler) {
  endpoint_panel_status->set_transition_type_handler(transition_type_handler);
}

void Switcher::set_panel_status_update_handler(
    std::function<void(nlohmann::json)> panel_status_update_handler) {
  endpoint_panel_status->set_panel_status_update_handler(panel_status_update_handler);
}

void Switcher::set_proxy_sources_update_handler(
    std::function<void(std::vector<int>)> proxy_sources_update_handler) {
  endpoint_panel_status->set_proxy_sources_update_handler(proxy_sources_update_handler);
}

void Switcher::set_get_key_status_handler(
    std::function<void(nlohmann::json)> get_key_status_handle) {
  endpoint_panel_status->set_get_key_status_handler(get_key_status_handle);
}

void Switcher::set_config_update_notify_handler(
    std::function<void(nlohmann::json)> config_update_notify_handler) {
  endpoint_config_update->set_config_update_notify_handler(config_update_notify_handler);
}

}// namespace seeder