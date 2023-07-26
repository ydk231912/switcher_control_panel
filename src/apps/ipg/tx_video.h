
//#define _GNU_SOURCE
#include "app_base.h"

#ifndef _TX_APP_VIDEO_HEAD_H_
#define _TX_APP_VIDEO_HEAD_H_
int st_app_tx_video_sessions_init(struct st_app_context* ctx);

int st_app_tx_video_sessions_stop(struct st_app_context* ctx);

int st_app_tx_video_sessions_uinit(struct st_app_context* ctx);

int st_app_tx_video_sessions_result(struct st_app_context* ctx);

int st_tx_video_source_init(struct st_app_context* ctx, st_json_context_t *json_ctx);

int st_app_tx_video_sessions_add(st_app_context* ctx, st_json_context_t *new_json_ctx);

int st_tx_video_source_start(st_app_context *ctx, seeder::core::input *source);

int st_app_tx_video_session_uinit(struct st_app_tx_video_session* s);

void st_app_tx_video_session_reset_stat(struct st_app_tx_video_session* s);


#endif
