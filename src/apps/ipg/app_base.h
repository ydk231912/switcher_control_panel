/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */
#pragma once

#include <json/value.h>
#include <mutex>
#include <unordered_map>
#ifdef APP_HAS_SDL2_TTF
#include <SDL2/SDL_ttf.h>
#endif
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <mtl/st20_api.h>
#include <mtl/st30_api.h>
#include <mtl/st40_api.h>
#include <mtl/mtl_api.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <vector>
#include <memory>

#include <string>
#include <atomic>

#include "app_platform.h"
#include "fmt.h"
#include "parse_json.h"

#include "core/stream/input.h"
#include "core/stream/output.h"
#include "core/video_format.h"

#define ST_APP_MAX_TX_VIDEO_SESSIONS (180)
#define ST_APP_MAX_TX_AUDIO_SESSIONS (180)
#define ST_APP_MAX_TX_ANC_SESSIONS (180)

#define ST_APP_MAX_RX_VIDEO_SESSIONS (180)
#define ST_APP_MAX_RX_AUDIO_SESSIONS (180)
#define ST_APP_MAX_RX_ANC_SESSIONS (180)

#define ST_APP_MAX_LCORES (32)

#define ST_APP_EXPECT_NEAR(val, expect, delta) \
  ((val > (expect - delta)) && (val < (expect + delta)))

#ifndef NS_PER_S
#define NS_PER_S (1000000000)
#endif

#define UTC_OFFSSET (37) /* 2022/07 */


struct st_app_frameinfo {
  bool used;
  bool second_field;
  uint16_t lines_ready;
};

struct st_app_tx_source
{
  std::string id; // uuid
  std::string type; // decklink, file
  int device_id; // decklink
  std::string file_url;
  std::string video_format;
  std::string pixel_format;
};

struct st_app_rx_output
{
  std::string id; // uuid
  std::string type; // decklink, file
  int device_id; // decklink
  std::string file_url;
  std::string video_format;
  std::string pixel_format;
};
struct st_app_tx_output_sdi
{
  std::string id; // uuid,用于绑定对应tx_source
  int device_id;  // decklink
  bool enable;
};

struct st_app_tx_video_session {
  int idx;
  std::string tx_source_id;
  mtl_handle st;
  st20_tx_handle handle;
  int handle_sch_idx;

  struct st_app_context* ctx;

  uint16_t framebuff_cnt;
  uint16_t framebuff_producer_idx;
  uint16_t framebuff_consumer_idx;
  struct st_tx_frame* framebuffs;

  int st20_frame_size;
  bool st20_second_field;
  struct st20_pgroup st20_pg;
  uint16_t lines_per_slice;
  enum st20_fmt pixel_format;

  int width;
  int height;
  int full_height; // for interlaced video
  int linesize;
  bool interlaced;
  bool second_field;
  bool single_line;
  bool slice;

  /* rtp info */
  bool st20_rtp_input;
  int st20_pkts_in_line;  /* GPM only, number of packets per each line, 4 for 1080p */
  int st20_bytes_in_line; /* bytes per line, 4800 for 1080p yuv422 10bit */
  uint32_t st20_pkt_data_len; /* data len(byte) for each pkt, 1200 for 1080p yuv422 10bit */
  struct st20_rfc4175_rtp_hdr st20_rtp_base;
  int st20_total_pkts;  /* total pkts in one frame, ex: 4320 for 1080p */
  int st20_pkt_idx;     /* pkt index in current frame */
  uint32_t st20_seq_id; /* seq id in current frame */
  uint32_t st20_rtp_tmstamp;
  uint8_t payload_type;

  double expect_fps;
  uint64_t stat_frame_frist_tx_time;
  uint32_t st20_frame_done_cnt;
  uint32_t st20_packet_done_cnt;

  pthread_t st20_app_thread;
  bool st20_app_thread_stop;
  pthread_cond_t st20_wake_cond;
  pthread_mutex_t st20_wake_mutex;

  int lcore;

  // video source
  std::shared_ptr<seeder::core::input> tx_source;
  std::shared_ptr<st_app_tx_source> source_info;

  // sdi_outoput 转播存储信息
  std::unordered_map<std::string, std::shared_ptr<seeder::core::output>> sdi_output;
  std::unordered_map<std::string, st_app_tx_output_sdi> sdi_output_info;

  std::atomic<int> next_frame_stat = 0;
  std::atomic<int> next_frame_not_ready_stat = 0;
  std::atomic<int> frame_done_stat = 0;
  std::atomic<int> build_frame_stat = 0;
  std::unique_ptr<uint8_t[]> half_height_buffer;
};

struct st_app_tx_audio_session {
  int idx;
  st30_tx_handle handle;
  std::string tx_source_id;
  uint16_t framebuff_cnt;
  uint16_t framebuff_producer_idx;
  uint16_t framebuff_consumer_idx;
  struct st_tx_frame* framebuffs;
  int handle_sch_idx;

  int st30_frame_done_cnt;
  int st30_packet_done_cnt;

  char st30_source_url[ST_APP_URL_MAX_LEN + 1];
  int st30_source_fd;
  uint8_t* st30_source_begin;
  uint8_t* st30_source_end;
  uint8_t* st30_frame_cursor; /* cursor to current frame */
  int st30_frame_size;
  int pkt_len;
  pthread_t st30_app_thread;
  bool st30_app_thread_stop;
  pthread_cond_t st30_wake_cond;
  pthread_mutex_t st30_wake_mutex;
  uint32_t st30_rtp_tmstamp;
  uint16_t st30_seq_id;

    // audio source
  std::shared_ptr<seeder::core::input> tx_source;
  std::shared_ptr<st_app_tx_source> source_info;

  std::atomic<int> next_frame_stat = 0;
  std::atomic<int> next_frame_not_ready_stat = 0;
  std::atomic<int> frame_done_stat = 0;
  std::atomic<int> build_frame_stat = 0;
};

struct st_app_rx_video_session {
  int idx;
  std::string rx_output_id;
  mtl_handle st;
  st20_rx_handle handle;
  int handle_sch_idx;
  int framebuff_cnt;
  int st20_frame_size;
  bool slice;
  int lcore;
  st_app_context *ctx;

  /* frame info */
  uint16_t framebuff_producer_idx;
  uint16_t framebuff_consumer_idx;
  struct st_rx_frame* framebuffs;

  /* rtp info */
  uint32_t st20_last_tmstamp;
  struct st20_pgroup st20_pg;
  struct user_pgroup user_pg;
  int width;
  int height;
  int full_height; // for interlaced video
  int linesize;
  bool interlaced;

  /* stat for Console Log */
  int stat_frame_received;
  uint64_t stat_last_time;
  int stat_frame_total_received;
  uint64_t stat_frame_frist_rx_time;
  double expect_fps;

  pthread_t st20_app_thread;
  pthread_cond_t st20_wake_cond;
  pthread_mutex_t st20_wake_mutex;
  bool st20_app_thread_stop;

  bool measure_latency;
  uint64_t stat_latency_us_sum;

  uint64_t stat_encode_us_sum;

  // video output handle
  std::shared_ptr<seeder::core::output> rx_output;
  st_app_rx_output output_info;
  std::unique_ptr<uint8_t[]> full_height_buffer;

  std::atomic<int> frame_receive_stat = 0; // for HTTP API
  std::atomic<int> frame_drop_stat = 0;
  std::atomic<int> frame_receive_status_cnt = 0;
};

struct st_app_rx_audio_session {
  int idx;
  std::string rx_output_id;
  st30_rx_handle handle;
  mtl_handle st;
  st_app_context *ctx;
  int lcore;
  int framebuff_cnt;
  int st30_frame_size;
  int pkt_len;

  char st30_ref_url[ST_APP_URL_MAX_LEN + 1];
  int st30_ref_fd;
  uint8_t* st30_ref_begin;
  uint8_t* st30_ref_end;
  uint8_t* st30_ref_cursor;

  pthread_t st30_app_thread;
  pthread_cond_t st30_wake_cond;
  pthread_mutex_t st30_wake_mutex;
  bool st30_app_thread_stop;

    /* frame info */
  uint16_t framebuff_producer_idx;
  uint16_t framebuff_consumer_idx;
  struct st_rx_frame* framebuffs;

  /* stat for Console Log */
  int stat_frame_total_received;
  uint64_t stat_frame_frist_rx_time;
  double expect_fps;

  // audio output handle
  std::shared_ptr<seeder::core::output> rx_output;
  st_app_rx_output output_info;
  int sample_num; // audio sample number per frame

  std::atomic<int> frame_receive_stat = 0; // for HTTP API
  std::atomic<int> frame_drop_stat = 0;
};

struct st_app_context {
  std::shared_ptr<st_json_context_t> json_ctx;
  std::string config_file_path;
  std::string nmos_node_config_file;
  struct mtl_init_params para;
  mtl_handle st;
  int test_time_s;
  bool stop;

  int lcore[ST_APP_MAX_LCORES];
  int rtp_lcore[ST_APP_MAX_LCORES];

  bool runtime_session;
  bool enable_hdr_split;
  bool tx_copy_once;
  bool app_thread;
  uint32_t rx_max_width;
  uint32_t rx_max_height;

  std::vector<std::shared_ptr<struct st_app_tx_video_session>> tx_video_sessions;
  std::vector<std::shared_ptr<st_app_tx_audio_session>> tx_audio_sessions;

  std::vector<std::shared_ptr<struct st_app_rx_video_session>> rx_video_sessions;
  std::vector<std::shared_ptr<struct st_app_rx_audio_session>> rx_audio_sessions;

  std::string decklink_id_map;
  int utc_offset;

  std::mutex mutex;
  // tx_source handle;
  std::unordered_map<std::string, std::shared_ptr<seeder::core::input>> tx_sources;
  std::unordered_map<std::string, st_app_tx_source> source_info;
  // rx_output handle;
  std::unordered_map<std::string, std::shared_ptr<seeder::core::output>> rx_output;
  std::unordered_map<std::string, st_app_rx_output> output_info;
  int http_port = 0;
  int next_tx_video_session_idx = 0;
  int next_tx_audio_session_idx = 0;
  int next_rx_video_session_idx = 0;
  int next_rx_audio_session_idx = 0;

  Json::Value stat;

  ~st_app_context();
};

static inline void* st_app_malloc(size_t sz) { return malloc(sz); }

static inline void* st_app_zmalloc(size_t sz) {
  void* p = malloc(sz);
  if (p) memset(p, 0x0, sz);
  return p;
}

static inline void st_app_free(void* p) { free(p); }

/* Monotonic time (in nanoseconds) since some unspecified starting point. */
static inline uint64_t st_app_get_monotonic_time() {
  struct timespec ts;

  clock_gettime(ST_CLOCK_MONOTONIC_ID, &ts);
  return ((uint64_t)ts.tv_sec * NS_PER_S) + ts.tv_nsec;
}

int st_app_video_get_lcore(struct st_app_context* ctx, int sch_idx, bool rtp,
                           unsigned int* lcore);

void st_set_video_foramt(const struct st_json_audio_info &info, seeder::core::video_format_desc* desc);

std::string st_app_ip_addr_to_string(void *addr);