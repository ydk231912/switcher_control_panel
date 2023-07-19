/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */
#include <math.h>
#include <mtl/st30_api.h>
#include <mtl/st40_api.h>
#include <mtl/st_pipeline_api.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <system_error>
#include <vector>
#include <memory>

#include "app_platform.h"
#include "core/video_format.h"
#include "fmt.h"
#include <json/json.h>
#ifndef _ST_APP_PARSE_JSON_HEAD_H_
#define _ST_APP_PARSE_JSON_HEAD_H_
// #if defined(__cplusplus)
// extern "C" {
// #endif



#define ST_APP_URL_MAX_LEN (256)

enum pacing {
  PACING_GAP,
  PACING_LINEAR,
  PACING_MAX,
};

enum tr_offset {
  TR_OFFSET_DEFAULT,
  TR_OFFSET_NONE,
  TR_OFFSET_MAX,
};

enum video_format {
  VIDEO_FORMAT_480I_59FPS,
  VIDEO_FORMAT_576I_50FPS,
  VIDEO_FORMAT_720P_119FPS,
  VIDEO_FORMAT_720P_59FPS,
  VIDEO_FORMAT_720P_50FPS,
  VIDEO_FORMAT_720P_29FPS,
  VIDEO_FORMAT_720P_25FPS,
  VIDEO_FORMAT_720P_60FPS,
  VIDEO_FORMAT_720P_30FPS,
  VIDEO_FORMAT_1080P_119FPS,
  VIDEO_FORMAT_1080P_59FPS,
  VIDEO_FORMAT_1080P_50FPS,
  VIDEO_FORMAT_1080P_29FPS,
  VIDEO_FORMAT_1080P_25FPS,
  VIDEO_FORMAT_1080I_59FPS,
  VIDEO_FORMAT_1080I_50FPS,
  VIDEO_FORMAT_1080P_60FPS,
  VIDEO_FORMAT_1080P_30FPS,
  VIDEO_FORMAT_2160P_119FPS,
  VIDEO_FORMAT_2160P_59FPS,
  VIDEO_FORMAT_2160P_50FPS,
  VIDEO_FORMAT_2160P_29FPS,
  VIDEO_FORMAT_2160P_25FPS,
  VIDEO_FORMAT_2160P_60FPS,
  VIDEO_FORMAT_2160P_30FPS,
  VIDEO_FORMAT_4320P_119FPS,
  VIDEO_FORMAT_4320P_59FPS,
  VIDEO_FORMAT_4320P_50FPS,
  VIDEO_FORMAT_4320P_29FPS,
  VIDEO_FORMAT_4320P_25FPS,
  VIDEO_FORMAT_4320P_60FPS,
  VIDEO_FORMAT_4320P_30FPS,
  VIDEO_FORMAT_AUTO,
  VIDEO_FORMAT_MAX,
};

enum anc_format {
  ANC_FORMAT_CLOSED_CAPTION,
  ANC_FORMAT_MAX,
};

struct st_video_fmt_desc {
  enum video_format fmt;
  enum seeder::core::video_fmt core_fmt;
  const char* name;
  uint32_t width;
  uint32_t height;
  enum st_fps fps;
};

typedef struct st_json_interface {
  char name[MTL_PORT_MAX_LEN];
  uint8_t ip_addr[MTL_IP_ADDR_LEN];
} st_json_interface_t;

typedef struct st_json_session_base {
  std::string id;
  uint8_t ip[MTL_PORT_MAX][MTL_IP_ADDR_LEN];
  st_json_interface_t inf[MTL_PORT_MAX];
  int num_inf;
  uint16_t udp_port;
  uint8_t payload_type;
} st_json_session_base_t;

typedef struct st_json_video_info {
  enum video_format video_format;
  enum pacing pacing;
  enum st20_type type;
  enum st20_packing packing;
  enum tr_offset tr_offset;
  enum st20_fmt pg_format;

  char video_url[ST_APP_URL_MAX_LEN];
} st_json_video_info_t;

typedef struct st_json_audio_info {
  enum st30_type type;
  enum st30_fmt audio_format;
  int audio_channel;
  enum st30_sampling audio_sampling;
  enum st30_ptime audio_ptime;

  char audio_url[ST_APP_URL_MAX_LEN];
} st_json_audio_info_t;

typedef struct st_json_ancillary_info {
  enum st40_type type;
  enum anc_format anc_format;
  enum st_fps anc_fps;

  char anc_url[ST_APP_URL_MAX_LEN];
} st_json_ancillary_info_t;

typedef struct st_json_st22p_info {
  enum st_frame_fmt format;
  enum pacing pacing;
  uint32_t width;
  uint32_t height;
  enum st_fps fps;
  enum st_plugin_device device;
  enum st22_codec codec;
  enum st22_pack_type pack_type;
  enum st22_quality_mode quality;
  uint32_t codec_thread_count;

  char st22p_url[ST_APP_URL_MAX_LEN];
} st_json_st22p_info_t;

typedef struct st_json_st20p_info {
  enum st_frame_fmt format;
  enum st20_fmt transport_format;
  enum pacing pacing;
  uint32_t width;
  uint32_t height;
  enum st_fps fps;
  enum st_plugin_device device;

  char st20p_url[ST_APP_URL_MAX_LEN];
} st_json_st20p_info_t;

typedef struct st_json_video_session {
  st_json_session_base_t base;
  st_json_video_info_t info;

  // tx only
  std::string tx_source_id; // tx_source.id, same as this.base.id
  
  /* rx only items */
  enum user_pg_fmt user_pg_format;
  bool display;
  bool measure_latency;

  std::string rx_output_id; // rx_output.id, same as this.base.id
} st_json_video_session_t;

typedef struct st_json_audio_session {
  st_json_session_base_t base;
  st_json_audio_info_t info;

  // tx_source.id
  std::string tx_source_id;
  // rx_ourput.id
  std::string rx_output_id;

} st_json_audio_session_t;

typedef struct st_json_ancillary_session {
  st_json_session_base_t base;
  st_json_ancillary_info_t info;
} st_json_ancillary_session_t;

typedef struct st_json_st22p_session {
  st_json_session_base_t base;
  st_json_st22p_info_t info;

  bool display;
  bool measure_latency;
} st_json_st22p_session_t;

typedef struct st_json_st20p_session {
  st_json_session_base_t base;
  st_json_st20p_info_t info;

  bool display;
  bool measure_latency;
} st_json_st20p_session_t;

typedef struct st_json_context {
  std::vector<st_json_interface_t> interfaces;
  int sch_quota;

  std::vector<st_json_video_session_t> tx_video_sessions;
  std::vector<st_json_audio_session_t> tx_audio_sessions;
  std::vector<st_json_ancillary_session_t> tx_anc_sessions;
  std::vector<st_json_st22p_session_t> tx_st22p_sessions;
  std::vector<st_json_st20p_session_t> tx_st20p_sessions;

  std::vector<st_json_video_session_t> rx_video_sessions;
  std::vector<st_json_audio_session_t> rx_audio_sessions;
  std::vector<st_json_ancillary_session_t> rx_anc_sessions;
  std::vector<st_json_st22p_session_t> rx_st22p_sessions;
  std::vector<st_json_st20p_session_t> rx_st20p_sessions;

  std::vector<struct st_app_tx_source> tx_sources;
  std::vector<struct st_app_rx_output> rx_output;

  Json::Value json_root;

  ~st_json_context();
} st_json_context_t;

std::error_code st_app_parse_json(st_json_context_t* ctx, const char* filename);

enum st_fps st_app_get_fps(enum video_format fmt);
uint32_t st_app_get_width(enum video_format fmt);
uint32_t st_app_get_height(enum video_format fmt);
bool st_app_get_interlaced(enum video_format fmt);

std::error_code st_app_parse_json_add(st_json_context_t* ctx, const std::string &json, std::unique_ptr<st_json_context_t> &new_ctx);
std::error_code st_app_parse_json_update(st_json_context_t* ctx, const std::string &json, std::unique_ptr<st_json_context_t> &new_ctx);

Json::Value st_app_get_fmts(st_json_context_t* ctx);


// #if defined(__cplusplus)
// }
// #endif

#endif
