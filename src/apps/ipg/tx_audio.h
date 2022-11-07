
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

#ifndef _TX_APP_AUDIO_HEAD_H_
#define _TX_APP_AUDIO_HEAD_H_
int st_app_tx_audio_sessions_init(struct st_app_context* ctx);

int st_app_tx_audio_sessions_stop(struct st_app_context* ctx);

int st_app_tx_audio_sessions_uinit(struct st_app_context* ctx);

int st_app_tx_audio_sessions_result(struct st_app_context* ctx);

#endif
