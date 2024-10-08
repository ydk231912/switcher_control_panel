#include "switcher.h"
#include <cstdio>
#include <fstream>
#include <string>
#include "commumication/ascll_convert_16_demo.h"
#include "commumication/uart_send.h"

namespace seeder {
  
// config::Config screen_config;
std::shared_ptr<seeder::ASCIIConvert16> ascll_convert_16;

Switcher::Switcher(std::string config_file_path)
    : config_file(config_file_path) {}

bool Switcher::init() {
  if (config_file.empty()) {
  }

  //读取配置文件
  std::ifstream file(config_file);
  config = nlohmann::json::parse(file);

  //提取配置参数
  std::string http_url = config["switcher"]["http_url"];
  std::string ws_url = config["switcher"]["ws_url"];
  int http_connection_timeout = config["switcher"]["http_connection_timeout"];
  int http_write_timeout = config["switcher"]["http_write_timeout"];
  int http_read_timeout = config["switcher"]["http_read_timeout"];
  
  screens_path = config["switcher"]["screen_path"];

  // 初始化 HTTP 客户端
  http_client = std::make_shared<HttpClient>(http_url);
  http_client->cli_->set_connection_timeout(0,http_connection_timeout);
  http_client->cli_->set_write_timeout(0,http_write_timeout);
  http_client->cli_->set_read_timeout(0,http_read_timeout);

  // /初始化 WebSocket 端点
  endpoint = std::make_shared<seeder::websocket_endpoint>();
  endpoint->connect(ws_url);

  return true;
}

nlohmann::json Switcher::get_screens_config(const std::string& screen_path){
  nlohmann::json screens_config;
  screens_config = http_client->get_json(screen_path);
  return screens_config;
}

std::vector<std::vector<std::vector<std::string>>> Switcher::store_screens_config() {
  nlohmann::json config_srceen = get_screens_config(screens_path);
  std::vector<std::vector<std::vector<std::string>>> hexVector(2, std::vector<std::vector<std::string>>(36, std::vector<std::string>(5)));
  int i,j = 0;
  for(auto &source : config_srceen["pgm"]["sources"]){
    screen_config.pgm_name.push_back(source["name"]);
  }
  for(auto &source : config_srceen["pvw"]["sources"]){
    screen_config.pvw_name.push_back(source["name"]);
  }
  
  for(i=0;i<36;i++){
    hexVector[0][i] = ascll_convert_16->asciiToHex(screen_config.pgm_name[i]);
  }
  for(i=0;i<36;i++){
    hexVector[1][i] = ascll_convert_16->asciiToHex(screen_config.pvw_name[i]);
  }
  
  std::vector<std::vector<std::vector<std::string>>> data(NUM_SLAVES, std::vector<std::vector<std::string>>(NUM_SCREENS, std::vector<std::string>(SCREEN_DATA_SIZE, "")));
  // 修改OLED显示屏数据
  const int rows = 2;
  const int depth = 5;
  const int sections = 3;
  constexpr int BUTTONS_PER_ROW = 4;
  constexpr int BYTES_PER_BUTTON = 5;
  for (size_t section = 0; section < sections; ++section) {
      for (size_t row = 0; row < rows; ++row) {
          for (size_t col = 0; col < BUTTONS_PER_ROW; ++col) {
              for (size_t depthIdx = 0; depthIdx < depth; ++depthIdx) {
                  size_t dataIndex = row * BUTTONS_PER_ROW * BYTES_PER_BUTTON + col * BYTES_PER_BUTTON + depthIdx;
                  size_t displayCol = section * BUTTONS_PER_ROW + col;
                  // data[0][section][dataIndex] = "0x31";
                  // data[1][section][dataIndex] = "0x54";
                  data[0][section][dataIndex] = hexVector[row][displayCol][depthIdx];
                  data[1][section][dataIndex] = hexVector[row][displayCol + 12][depthIdx];
                  data[2][section][dataIndex] = hexVector[row][displayCol + 24][depthIdx];
                  data[3][section][dataIndex] = hexVector[row][displayCol][depthIdx];
                  data[4][section][dataIndex] = hexVector[row][displayCol + 12][depthIdx];
                  data[5][section][dataIndex] = hexVector[row][displayCol + 24][depthIdx];
              }
          }
      }
  }

  return data;
}


//pgm
void Switcher::pgm(int idx) {
  //创建和配置 JSON 对象
  nlohmann::json XPT = nlohmann::json::parse(R"(
  {
    "id": "xpt",
    "source": 0,
    "target": "PGM",
    "type_name": "XPTWrite"
}
)");

//更新 JSON 对象
  XPT["source"] = idx;
  XPT["target"] = "PGM";
  logger->info("pgm : {}", idx);
  //发送 HTTP POST 请求
  http_client->post("/api/panel/mixer_command", XPT.dump());
}

//pvw
void Switcher::pvw(int idx) {
  nlohmann::json XPT = nlohmann::json::parse(R"(
  {
    "id": "xpt",
    "source": 0,
    "target": "PVW",
    "type_name": "XPTWrite"
}
)");
  XPT["source"] = idx;
  XPT["target"] = "PVW";
  logger->info("pvw : {}", idx);
  http_client->post("/api/panel/mixer_command", XPT.dump());
}

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

//mix
void Switcher::mix(const nlohmann::json &type) {
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
//Dip、Wipe、Stinger、DVE

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
  command["frame_num"] = config["switcher"]["auto_transition_frame_num"];
  command["sec_num"] = config["switcher"]["auto_transition_sec_num"];
  logger->info("auto transition");
  http_client->post("/api/panel/mixer_command", command.dump());
}

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
  endpoint->set_panel_status_handler(panel_status_handler);
}
void Switcher::set_dsk_status_handler(
    std::function<void(std::vector<bool>)> dsk_status_handler) {
  endpoint->set_dsk_status_handler(dsk_status_handler);
}

void Switcher::set_transition_type_handler(
    std::function<void(std::string)> transition_type_handler) {
  endpoint->set_transition_type_handler(transition_type_handler);
}

void Switcher::set_next_transition_handler(
    std::function<void(std::vector<bool>)> next_transition_handler) {
  endpoint->set_next_transition_handler(next_transition_handler);
}

void Switcher::set_proxy_type_handler(
    std::function<void(std::vector<bool>)> proxy_type_handler) {
  endpoint->set_proxy_type_handler(proxy_type_handler);
}

void Switcher::set_proxy_source_handler(
    std::function<void(std::vector<bool>)> proxy_source_handler) {
  endpoint->set_proxy_source_handler(proxy_source_handler);
}

void Switcher::set_mode_status_handler(
    std::function<void(std::vector<bool>)> mode_status_handler) {
  endpoint->set_mode_status_handler(mode_status_handler);
}


void Switcher::set_shift_status_handler(
    std::function<void(std::vector<bool>)> shift_status_handler) {
  endpoint->set_shift_status_handler(shift_status_handler);
}

} // namespace seeder
