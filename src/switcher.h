#pragma once

#include <boost/mpl/bool.hpp>
#include <functional>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <json.hpp>
#include <vector>
#include "http_client.h"
#include "http_server.h"
#include "websocket_client.h"
#include "commumication/config.h"



namespace seeder {
// Logger
extern std::shared_ptr<spdlog::logger> logger;

class Switcher {
public:
  explicit Switcher(std::string config_file);
  ~Switcher();

  bool init();
  void stop();
  void loadConfig(nlohmann::json &config);

  nlohmann::json get_screens_config(const std::string &screen_path);
  void store_screens_config(
    std::vector<std::vector<std::vector<std::string>>> &screen_data,
    nlohmann::json &pgm_commands, nlohmann::json &pvw_commands, nlohmann::json &proxy_keys_commands);
  void get_hex_vector(const std::vector<std::string>& names, 
    std::vector<std::vector<std::string>>& hexVector, int shift_status, int panel_config_num);
  void update_oled_display(std::vector<std::vector<std::vector<std::string>>>& data, 
  const std::vector<std::vector<std::vector<std::string>>>& hexVector, int switcher_keyboard_id);

  void execute_xpt_command(const nlohmann::json &command);
  void execute_on_air_command(const nlohmann::json &command);
  void xpt_pgm(int index);
  void xpt_pvw(int index);
  // void on_air_tie_fill(int index);
  
  void cut();
  void auto_();

  void set_transition(const nlohmann::json &type);

  void mixer_command_manual(const nlohmann::json &is_auto);
  void mixer_command_manual_ratio(int progress_ratio);

  void prevtrans();
  void dsk(int dsk_index);
  void nextkey(int dsk_index);
  void bkgd();

  void mode();
  void proxy_key();
  void proxy_key_source(int key_source_index);
  void proxy_source(int source_index , const std::string& source) ;
  void proxy_aux();
  void proxy_aux_source(int aux_source_index);
  void proxy_utl();
  void proxy_utl_source(int utl_source_index);
  void proxy_macro();
  void proxy_macro_source(int macro_source_index);
  void proxy_snapshot();
  void proxy_snapshot_source(int snapshot_source_index);
  void proxy_m_e();
  void proxy_m_e_source(int m_e_source_index);
  void shift(const std::string& source);

  void proxy(const std::string& source);
  void proxy_sources(int key_source_index , const std::string& source);
  
  void
  set_panel_status_handler(std::function<void(int, int)> panel_status_handler);
  void 
  set_proxy_sources_update_handler(std::function<void(std::vector<int>)> proxy_sources_update_handler);
  void 
  set_dsk_status_handler(std::function<void(std::vector<bool>)> dsk_status_handler);
  void 
  set_transition_type_handler(std::function<void(std::string)> transition_type_handler);
  void 
  set_panel_status_update_handler(std::function<void(nlohmann::json)> panel_status_update_handler);
  void 
  set_get_key_status_handler(std::function<void(nlohmann::json)> get_key_status_handle);
  void 
  set_config_update_notify_handler(std::function<void(nlohmann::json)> config_update_notify_handler);
  

  nlohmann::json config;

  std::shared_ptr<config::Config> control_panel_config;

  std::string config_file;

  std::shared_ptr<HttpClient> http_client;
  std::shared_ptr<HttpServer> http_service;
  
  std::shared_ptr<seeder::websocket_endpoint> endpoint_panel_status;
  std::shared_ptr<seeder::websocket_endpoint> endpoint_config_update;
  nlohmann::json pgm_commands;
  nlohmann::json pvw_commands;
  nlohmann::json proxy_keys_commands;
  
  int pgm_source_sum = 0;
  int pvw_source_sum = 0;
  int proxy_sum = 0;
  int proxy_sources_sum = 0;

  int pgm_shift_status = 0;
  int pvw_shift_status = 0;
  int proxy_source_shift_status = 0;
  
  std::vector<int> sw_pgm_pvw_shift_status = {0,0};

private:
  std::vector<std::string> shift_name = {"1st","2nd","3rd","4ur"};
  std::vector<std::string> surrogate_key_type_name = {"KEY","AUX","UTL","MACRO","SNAPSHOT","M/E"};
};

}// namespace seeder