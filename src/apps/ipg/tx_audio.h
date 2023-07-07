
#pragma once

#include "app_base.h"

int st_app_tx_audio_sessions_init(struct st_app_context* ctx);

int st_app_tx_audio_sessions_stop(struct st_app_context* ctx);

int st_app_tx_audio_sessions_uinit(struct st_app_context* ctx);

int st_app_tx_audio_session_uinit(struct st_app_tx_audio_session* s);

int st_app_tx_audio_sessions_result(struct st_app_context* ctx);

int st_app_tx_audio_sessions_add(struct st_app_context* ctx, st_json_context_t *new_json_ctx);

// int st_app_tx_audio_sessions_uinit_update(struct st_app_context* ctx,int id,st_json_context_t* c);
// int st_app_tx_audio_sessions_init_add(struct st_app_context* ctx,st_json_context_t* ctx_add);
// int st_app_tx_audio_sessions_init_update(struct st_app_context* ctx,st_json_context_t* c);
