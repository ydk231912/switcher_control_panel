
//#define _GNU_SOURCE
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "app_base.h"
extern "C"
{
    
}

#ifndef _TX_APP_VIDEO_HEAD_H_
#define _TX_APP_VIDEO_HEAD_H_
int st_app_tx_video_sessions_init(struct st_app_context* ctx);

int st_app_tx_video_sessions_stop(struct st_app_context* ctx);

int st_app_tx_video_sessions_uinit(struct st_app_context* ctx);

int st_app_tx_video_sessions_result(struct st_app_context* ctx);

int st_app_tx_video_sessions_uinit_update(struct st_app_context* ctx,int id,st_json_context_t* c);

int st_app_tx_video_sessions_init_add(struct st_app_context* ctx,st_json_context_t* c);

int st_app_tx_video_sessions_init_update(struct st_app_context* ctx,st_json_context_t* c);


#endif
