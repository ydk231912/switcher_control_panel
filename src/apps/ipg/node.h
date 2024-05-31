#pragma once
#include "app_base.h"
#include "core/util/logger.h"
#include "cpprest/grant_type.h"
#include "cpprest/token_endpoint_auth_method.h"
#include "nmos/api_utils.h" // for make_api_listener
#include "nmos/authorization_behaviour.h"
#include "nmos/authorization_redirect_api.h"
#include "nmos/authorization_state.h"
#include "nmos/control_protocol_state.h"
#include "nmos/id.h"
#include "nmos/jwks_uri_api.h"
#include "nmos/log_gate.h"
#include "nmos/model.h"
#include "nmos/node_server.h"
#include "nmos/ocsp_behaviour.h"
#include "nmos/ocsp_response_handler.h"
#include "nmos/ocsp_state.h"
#include "nmos/process_utils.h"
#include "nmos/server.h"
#include "nmos/server_utils.h" // for make_http_listener_config
#include "node_implementation.h"
#include "st_config_utils.h"
#include "string"
#include <cpprest/details/basic_types.h>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <nmos/id.h>
#include <nmos/mutex.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
const unsigned int delay_millis{0};
typedef std::unordered_map<nmos::id, std::string> session_map_t;
namespace seeder {

class nmos_node {
private:
  std::thread thread_;
  st_app_context *ctx_;
  std::string config_file_;
  nmos::node_model node_model_;
  nmos::experimental::log_gate *gate_;
  nmos::id node_id_;
  nmos::id device_id_;
  utility::string_t seed_id_;
  std::vector<nmos::id> sender_ids_;
  std::vector<nmos::id> receiver_ids_;
  std::vector<nmos::id> node_ids_;
  std::vector<nmos::id> device_ids_;
  std::vector<nmos::id> source_ids_;
  std::vector<nmos::id> flow_ids_;
  web::hosts::experimental::host_interface primary_interface;
  web::hosts::experimental::host_interface secondary_interface;
  session_map_t sender_video_session_map_;
  session_map_t receiver_video_session_map_;
  session_map_t sender_audio_session_map_;
  session_map_t receiver_audio_session_map_;
  nmos::connection_sender_transportfile_setter set_transportfile;
  nmos::connection_resource_auto_resolver resolve_auto;
  bool insert_resource_after(unsigned int milliseconds,
                             nmos::resources &resources,
                             nmos::resource &&resource, slog::base_gate &gate,
                             nmos::write_lock &lock);
  bool remove_resource_after(unsigned int milliseconds,
                             nmos::resources &resources, nmos::id id,
                             slog::base_gate &gate, nmos::write_lock &lock);
  nmos::experimental::node_implementation make_node_implementation();
  nmos::system_global_handler make_node_implementation_system_global_handler();
  nmos::registration_handler make_node_implementation_registration_handler();
  nmos::transport_file_parser make_node_implementation_transport_file_parser();
  nmos::connection_activation_handler
  make_node_implementation_connection_activation_handler();
  nmos::connection_resource_auto_resolver
  make_node_implementation_auto_resolver(const nmos::settings &settings);
  nmos::connection_sender_transportfile_setter
  make_node_implementation_transportfile_setter(
      const nmos::resources &node_resources, const nmos::settings &settings);
  void node_implementation_run();
  void init_sender();
  void init_device();
  void init_receiver();
  void init();
  void thread_run();
  int nmos_node_start();
  st_json_video_session_t find_tx_session_by_id(std::string id);
  st_json_video_session_t find_rx_session_by_id(std::string id);
  void add_audio_sender(std::string id);
  void add_video_sender(std::string id);
  void remove_audio_sender(std::string id, bool use_lock);
  void remove_video_sender(std::string id, bool use_lock);
  void udpate_video_sender(std::string id);
  void udpate_audio_sender(std::string id);

  void add_audio_receiver(std::string id);
  void add_video_receiver(std::string id);
  void remove_audio_receiver(std::string id, bool use_lock);
  void remove_video_receiver(std::string id, bool use_lock);
  void udpate_video_receiver(std::string id);
  void udpate_audio_receiver(std::string id);

  sdp::sampling st_get_color_sampling(st20_fmt fmt);
  int st_get_component_depth(st20_fmt fmt);

public:
  nmos_node(st_app_context *ctx_);
  ~nmos_node();
  void start();
  void add();
  void add_sender(std::string id);
  void add_receiver(std::string id);
  void remove_sender(std::string id, bool use_lock);
  void remove_receiver(std::string id, bool use_lock);
  void udpate_sender(std::string id);
  void update_receiver(std::string id);
};

} // namespace seeder
