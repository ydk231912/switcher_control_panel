#pragma once

#include <boost/mpl/bool.hpp>
#include <functional>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <json.hpp>
#include "http_client.h"
#include "websocket_client.h"
#include "commumication/config.h"



namespace seeder {
// Logger
extern std::shared_ptr<spdlog::logger> logger;

class Switcher {
public:
  Switcher(std::string config_file);
  bool init();

  nlohmann::json get_screens_config(const std::string &screen_path);
  std::vector<std::vector<std::vector<std::string>>> store_screens_config();

  void pgm(int idx);
  void pvw(int idx);
  void cut();
  void auto_();

  void mix(const nlohmann::json &type);
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
  set_dsk_status_handler(std::function<void(std::vector<bool>)> dsk_status_handler);
  void 
  set_transition_type_handler(std::function<void(std::string)> transition_type_handler);
  void 
  set_proxy_type_handler(std::function<void(std::vector<bool>)> proxy_type_handler);
  void 
  set_next_transition_handler(std::function<void(std::vector<bool>)> set_next_transition_handler);
  void 
  set_proxy_source_handler(std::function<void(std::vector<bool>)> proxy_source_handler);
  void 
  set_mode_status_handler(std::function<void(std::vector<bool>)> mode_status_handler);
  void 
  set_shift_status_handler(std::function<void(std::vector<bool>)> shift_status_handler);

  nlohmann::json config;

  config::Config screen_config;

  
  // std::shared_ptr<seeder::config::Config> screen_config;
private:
  std::string config_file;

  std::shared_ptr<HttpClient> http_client;
  std::shared_ptr<seeder::websocket_endpoint> endpoint;

  std::string screens_path;
  // int current_pgm = 0;
  // int current_pvw = 0;
};

}; // namespace seeder