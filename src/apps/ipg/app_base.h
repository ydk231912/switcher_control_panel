/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */
#pragma once

#include <SDL2/SDL.h>
#include <json/forwards.h>
#include <mutex>
#include <unordered_map>
#ifdef APP_HAS_SDL2_TTF
#include <SDL2/SDL_ttf.h>
#endif
#include <errno.h>
#include <pcap.h>
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

struct st_display {
  int idx;
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_PixelFormatEnum fmt;
#ifdef APP_HAS_SDL2_TTF
  TTF_Font* font;
#endif
  SDL_Rect msg_rect;
  int window_w;
  int window_h;
  int pixel_w;
  int pixel_h;
  void* front_frame;
  int front_frame_size;
  uint32_t last_time;
  uint32_t frame_cnt;
  double fps;

  pthread_t display_thread;
  bool display_thread_stop;
  pthread_cond_t display_wake_cond;
  pthread_mutex_t display_wake_mutex;
  pthread_mutex_t display_frame_mutex;
};

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

  pcap_t* st20_pcap;
  bool st20_pcap_input;

  char st20_source_url[ST_APP_URL_MAX_LEN];
  uint8_t* st20_source_begin;
  uint8_t* st20_source_end;
  uint8_t* st20_frame_cursor;
  int st20_source_fd;
  bool st20_frames_copied;

  int st20_frame_size;
  bool st20_second_field;
  struct st20_pgroup st20_pg;
  uint16_t lines_per_slice;

  int width;
  int height;
  bool interlaced;
  bool second_field;
  bool single_line;
  bool slice;

  /* rtp info */
  bool st20_rtp_input;
  int st20_pkts_in_line;  /* GPM only, number of packets per each line, 4 for 1080p */
  int st20_bytes_in_line; /* bytes per line, 4800 for 1080p yuv422 10bit */
  uint32_t
      st20_pkt_data_len; /* data len(byte) for each pkt, 1200 for 1080p yuv422 10bit */
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

  struct st_display* display;
  int lcore;

  // video source
  std::shared_ptr<seeder::core::input> tx_source;
  std::shared_ptr<st_app_tx_source> source_info;
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
  pcap_t* st30_pcap;
  bool st30_pcap_input;
  bool st30_rtp_input;
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
};

struct st_app_tx_anc_session {
  int idx;
  st40_tx_handle handle;

  uint16_t framebuff_cnt;

  uint16_t framebuff_producer_idx;
  uint16_t framebuff_consumer_idx;
  struct st_tx_frame* framebuffs;

  uint32_t st40_frame_done_cnt;
  uint32_t st40_packet_done_cnt;

  char st40_source_url[ST_APP_URL_MAX_LEN + 1];
  int st40_source_fd;
  pcap_t* st40_pcap;
  bool st40_pcap_input;
  bool st40_rtp_input;
  uint8_t* st40_source_begin;
  uint8_t* st40_source_end;
  uint8_t* st40_frame_cursor; /* cursor to current frame */
  pthread_t st40_app_thread;
  bool st40_app_thread_stop;
  pthread_cond_t st40_wake_cond;
  pthread_mutex_t st40_wake_mutex;
  uint32_t st40_rtp_tmstamp;
  uint32_t st40_seq_id;
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

  char st20_dst_url[ST_APP_URL_MAX_LEN];
  int st20_dst_fb_cnt; /* the count of recevied fbs will be saved to file */
  int st20_dst_fd;
  uint8_t* st20_dst_begin;
  uint8_t* st20_dst_end;
  uint8_t* st20_dst_cursor;

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

  /* stat */
  int stat_frame_received;
  uint64_t stat_last_time;
  int stat_frame_total_received;
  uint64_t stat_frame_frist_rx_time;
  double expect_fps;

  pthread_t st20_app_thread;
  pthread_cond_t st20_wake_cond;
  pthread_mutex_t st20_wake_mutex;
  bool st20_app_thread_stop;

  struct st_display* display;
  uint32_t pcapng_max_pkts;

  bool measure_latency;
  uint64_t stat_latency_us_sum;

  uint64_t stat_encode_us_sum;

  // video output handle
  std::shared_ptr<seeder::core::output> rx_output;
  st_app_rx_output output_info;
};

struct st_app_rx_audio_session {
  int idx;
  std::string rx_output_id;
  st30_rx_handle handle;
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

  /* stat */
  int stat_frame_total_received;
  uint64_t stat_frame_frist_rx_time;
  double expect_fps;

  // audio output handle
  std::shared_ptr<seeder::core::output> rx_output;
  st_app_rx_output output_info;
  int sample_num; // audio sample number per frame
};

struct st_app_rx_anc_session {
  int idx;
  st40_rx_handle handle;
  pthread_t st40_app_thread;
  pthread_cond_t st40_wake_cond;
  pthread_mutex_t st40_wake_mutex;
  bool st40_app_thread_stop;

  /* stat */
  int stat_frame_total_received;
  uint64_t stat_frame_frist_rx_time;
};

struct st22_app_tx_session {
  int idx;
  st22_tx_handle handle;

  int width;
  int height;
  enum st22_type type;
  int bpp;
  size_t bytes_per_frame;

  struct st_app_context* ctx;
  mtl_handle st;
  int lcore;
  int handle_sch_idx;

  uint16_t framebuff_cnt;
  uint16_t framebuff_producer_idx;
  uint16_t framebuff_consumer_idx;
  struct st_tx_frame* framebuffs;

  pthread_cond_t wake_cond;
  pthread_mutex_t wake_mutex;

  bool st22_app_thread_stop;
  pthread_t st22_app_thread;
  char st22_source_url[ST_APP_URL_MAX_LEN];
  int st22_source_fd;
  uint8_t* st22_source_begin;
  uint8_t* st22_source_end;
  uint8_t* st22_frame_cursor;

  int fb_send;
};

struct st22_app_rx_session {
  int idx;
  st22_rx_handle handle;
  int width;
  int height;
  int bpp;
  size_t bytes_per_frame;

  uint16_t framebuff_cnt;
  uint16_t framebuff_producer_idx;
  uint16_t framebuff_consumer_idx;
  struct st_rx_frame* framebuffs;

  pthread_cond_t wake_cond;
  pthread_mutex_t wake_mutex;

  bool st22_app_thread_stop;
  pthread_t st22_app_thread;
  int fb_decoded;

  char st22_dst_url[ST_APP_URL_MAX_LEN];
  int st22_dst_fb_cnt; /* the count of recevied fbs will be saved to file */
  int st22_dst_fd;
  uint8_t* st22_dst_begin;
  uint8_t* st22_dst_end;
  uint8_t* st22_dst_cursor;
};

struct st_app_tx_st22p_session {
  int idx;
  st22p_tx_handle handle;
  mtl_handle st;
  int framebuff_cnt;
  int st22p_frame_size;
  int width;
  int height;

  char st22p_source_url[ST_APP_URL_MAX_LEN];
  uint8_t* st22p_source_begin;
  uint8_t* st22p_source_end;
  uint8_t* st22p_frame_cursor;
  int st22p_source_fd;

  double expect_fps;

  pthread_t st22p_app_thread;
  pthread_cond_t st22p_wake_cond;
  pthread_mutex_t st22p_wake_mutex;
  bool st22p_app_thread_stop;
};

struct st_app_rx_st22p_session {
  int idx;
  mtl_handle st;
  st22p_rx_handle handle;
  int framebuff_cnt;
  int st22p_frame_size;
  bool slice;
  int width;
  int height;

  /* stat */
  int stat_frame_received;
  uint64_t stat_last_time;
  int stat_frame_total_received;
  uint64_t stat_frame_frist_rx_time;
  double expect_fps;

  pthread_t st22p_app_thread;
  pthread_cond_t st22p_wake_cond;
  pthread_mutex_t st22p_wake_mutex;
  bool st22p_app_thread_stop;

  struct st_display* display;
  uint32_t pcapng_max_pkts;

  bool measure_latency;
  uint64_t stat_latency_us_sum;
};

struct st_app_tx_st20p_session {
  int idx;
  st20p_tx_handle handle;
  mtl_handle st;
  int framebuff_cnt;
  int st20p_frame_size;
  int width;
  int height;

  char st20p_source_url[ST_APP_URL_MAX_LEN];
  uint8_t* st20p_source_begin;
  uint8_t* st20p_source_end;
  uint8_t* st20p_frame_cursor;
  int st20p_source_fd;

  double expect_fps;

  pthread_t st20p_app_thread;
  pthread_cond_t st20p_wake_cond;
  pthread_mutex_t st20p_wake_mutex;
  bool st20p_app_thread_stop;
};

struct st_app_rx_st20p_session {
  int idx;
  st20p_rx_handle handle;
  mtl_handle st;
  int framebuff_cnt;
  int st20p_frame_size;
  int width;
  int height;

  /* stat */
  int stat_frame_received;
  uint64_t stat_last_time;
  int stat_frame_total_received;
  uint64_t stat_frame_frist_rx_time;
  double expect_fps;

  pthread_t st20p_app_thread;
  pthread_cond_t st20p_wake_cond;
  pthread_mutex_t st20p_wake_mutex;
  bool st20p_app_thread_stop;

  struct st_display* display;
  uint32_t pcapng_max_pkts;

  bool measure_latency;
  uint64_t stat_latency_us_sum;
};

struct st_app_context {
  std::shared_ptr<st_json_context_t> json_ctx;
  std::string config_file_path;
  struct mtl_init_params para;
  mtl_handle st;
  int test_time_s;
  bool stop;
  uint8_t tx_dip_addr[MTL_PORT_MAX][MTL_IP_ADDR_LEN]; /* tx destination IP */
  bool has_tx_dst_mac[MTL_PORT_MAX];
  uint8_t tx_dst_mac[MTL_PORT_MAX][6];

  int lcore[ST_APP_MAX_LCORES];
  int rtp_lcore[ST_APP_MAX_LCORES];

  bool runtime_session;
  bool enable_hdr_split;
  bool tx_copy_once;
  bool app_thread;
  uint32_t rx_max_width;
  uint32_t rx_max_height;

  char tx_video_url[ST_APP_URL_MAX_LEN]; /* send video content url*/
  std::vector<std::shared_ptr<struct st_app_tx_video_session>> tx_video_sessions;
  int tx_video_rtp_ring_size; /* the ring size for tx video rtp type */

  std::vector<std::shared_ptr<st_app_tx_audio_session>> tx_audio_sessions;
  char tx_audio_url[ST_APP_URL_MAX_LEN];
  int tx_audio_rtp_ring_size; /* the ring size for tx audio rtp type */

  std::vector<std::shared_ptr<struct st_app_tx_anc_session>> tx_anc_sessions;
  char tx_anc_url[ST_APP_URL_MAX_LEN];
  int tx_anc_rtp_ring_size; /* the ring size for tx anc rtp type */

  char tx_st22p_url[ST_APP_URL_MAX_LEN]; /* send st22p content url*/
  std::vector<std::shared_ptr<struct st_app_tx_st22p_session>> tx_st22p_sessions;

  char tx_st20p_url[ST_APP_URL_MAX_LEN]; /* send st20p content url*/
  std::vector<std::shared_ptr<struct st_app_tx_st20p_session>> tx_st20p_sessions;

  uint8_t rx_sip_addr[MTL_PORT_MAX][MTL_IP_ADDR_LEN]; /* rx source IP */

  std::vector<std::shared_ptr<struct st_app_rx_video_session>> rx_video_sessions;
  int rx_video_file_frames; /* the frames recevied saved to file */
  int rx_video_fb_cnt;
  int rx_video_rtp_ring_size; /* the ring size for rx video rtp type */
  bool display;               /* flag to display all rx video with SDL */
  bool has_sdl;               /* has SDL device or not*/

  std::vector<std::shared_ptr<struct st_app_rx_audio_session>> rx_audio_sessions;
  int rx_audio_rtp_ring_size; /* the ring size for rx audio rtp type */

  std::vector<std::shared_ptr<struct st_app_rx_anc_session>> rx_anc_sessions;

  std::vector<std::shared_ptr<struct st_app_rx_st22p_session>> rx_st22p_sessions;

  std::vector<std::shared_ptr<struct st_app_rx_st20p_session>> rx_st20p_sessions;

  char tx_st22_url[ST_APP_URL_MAX_LEN]; /* send st22 content url*/
  std::vector<std::shared_ptr<struct st22_app_tx_session>> tx_st22_sessions;
  std::vector<std::shared_ptr<struct st22_app_rx_session>> rx_st22_sessions;
  int st22_bpp;

  uint32_t pcapng_max_pkts;
  char ttf_file[ST_APP_URL_MAX_LEN];
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

void st_set_video_foramt(struct st_json_audio_info info, seeder::core::video_format_desc* desc);