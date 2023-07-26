#include "app_base.h"

#ifndef _RX_APP_VIDEO_HEAD_H_
#define _RX_APP_VIDEO_HEAD_H_

int st_app_rx_video_sessions_init(struct st_app_context* ctx);

int st_app_rx_video_sessions_uinit(struct st_app_context* ctx);

int st_app_rx_video_session_uinit(struct st_app_rx_video_session* s);

int st_app_rx_video_sessions_add(struct st_app_context* ctx, st_json_context_t *new_json_ctx);

int st_app_rx_output_init(struct st_app_context* ctx, st_json_context *json_ctx);

int st_rx_output_start(struct st_app_context* ctx, seeder::core::output *output);

int st_app_rx_video_sessions_stat(struct st_app_context* ctx);

void st_app_rx_video_session_reset_stat(struct st_app_rx_video_session* s);

// int st_app_rx_video_sessions_result(struct st_app_context* ctx);

// int st_app_rx_video_sessions_pcap(struct st_app_context* ctx);


#endif