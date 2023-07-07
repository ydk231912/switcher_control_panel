#include "app_base.h"

#ifndef _RX_APP_AUDIO_HEAD_H_
#define _RX_APP_AUDIO_HEAD_H_

int st_app_rx_audio_sessions_init(struct st_app_context* ctx);

int st_app_rx_audio_sessions_uinit(struct st_app_context* ctx);

int st_app_rx_audio_sessions_result(struct st_app_context* ctx);


// int st_app_rx_audio_sessions_init_add(struct st_app_context* ctx,st_json_context_t* c);
// int st_app_rx_audio_sessions_init_update(struct st_app_context* ctx,st_json_context_t* c);

// int st_app_rx_audio_sessions_uinit_update(struct st_app_context* ctx,int id,st_json_context_t *c);


#endif