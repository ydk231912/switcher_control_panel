/**
 * @file main.cpp
 * @author
 * @brief seeder ipg: convert SDI data to ST2110, send data by udp,
 * receive ST2110 rtp convert to SID(decklink)
 * @version
 * @date 2022-10-25
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <string>
#include <future>
#include <exception>
#include <iostream>
#include <malloc.h>
#include <functional>

#include "core/util/logger.h"
#include "core/video_format.h"
#include "decklink/input/decklink_input.h"
#include "ffmpeg/input/ffmpeg_input.h"


#include "args.h"

#include "app_base.h"
#include "tx_video.h"
#include "tx_audio.h"

extern "C"
{
    #include "test.h"
}

// #include "core/video_format.h"
// #include "config.h"
// #include "server.h"

using namespace seeder::core;

static struct st_app_context* g_app_ctx; /* only for st_app_sig_handler */
static enum st_log_level app_log_level;

void app_set_log_level(enum st_log_level level) { app_log_level = level; }

enum st_log_level app_get_log_level(void) { return app_log_level; }

static void app_stat(void *priv)
{
    struct st_app_context *ctx = (st_app_context *)priv;

    // st_app_rx_video_sessions_stat(ctx);
    // st_app_rx_st22p_sessions_stat(ctx);
    // st_app_rx_st20p_sessions_stat(ctx);
}

static uint64_t app_ptp_from_tai_time(void *priv)
{
    struct st_app_context *ctx = (st_app_context *)priv;
    struct timespec spec;
    st_getrealtime(&spec);
    spec.tv_sec -= ctx->utc_offset;
    return ((uint64_t)spec.tv_sec * NS_PER_S) + spec.tv_nsec;
}

int st_app_video_get_lcore(struct st_app_context* ctx, int sch_idx, bool rtp,
                           unsigned int* lcore) 
{
    int ret;
    unsigned int video_lcore;

    if(sch_idx < 0 || sch_idx >= ST_APP_MAX_LCORES) 
    {
        logger->error("{}, invalid sch idx {}", __func__, sch_idx);
        return -EINVAL;
    }

    if(rtp) 
    {
        if(ctx->rtp_lcore[sch_idx] < 0) 
        {
            ret = st_get_lcore(ctx->st, &video_lcore);
            if(ret < 0) return ret;
            ctx->rtp_lcore[sch_idx] = video_lcore;
            logger->info("{}, new rtp lcore {} for sch idx {}", __func__, video_lcore, sch_idx);
        }
    }else 
    {
        if(ctx->lcore[sch_idx] < 0) 
        {
            ret = st_get_lcore(ctx->st, &video_lcore);
            if(ret < 0) return ret;
            ctx->lcore[sch_idx] = video_lcore;
            logger->info("{}, new lcore {} for sch idx {}", __func__, video_lcore, sch_idx);
        }
    }

    if(rtp)
        *lcore = ctx->rtp_lcore[sch_idx];
    else
        *lcore = ctx->lcore[sch_idx];
    return 0;
}

static void user_param_init(struct st_app_context *ctx, struct st_init_params *p)
{
    memset(p, 0x0, sizeof(*p));

    p->pmd[ST_PORT_P] = ST_PMD_DPDK_USER;
    p->pmd[ST_PORT_R] = ST_PMD_DPDK_USER;
    /* defalut start queue set to 1 */
    p->xdp_info[ST_PORT_P].start_queue = 1;
    p->xdp_info[ST_PORT_R].start_queue = 1;
    p->flags |= ST_FLAG_BIND_NUMA; /* default bind to numa */
    p->flags |= ST_FLAG_TX_VIDEO_MIGRATE;
    p->flags |= ST_FLAG_RX_VIDEO_MIGRATE;
    p->flags |= ST_FLAG_RX_SEPARATE_VIDEO_LCORE;
    p->priv = ctx;
    p->ptp_get_time_fn = app_ptp_from_tai_time;
    p->stat_dump_cb_fn = app_stat;
    p->log_level = ST_LOG_LEVEL_INFO;
    // app_set_log_level(p->log_level);
}

static void st_app_ctx_init(struct st_app_context *ctx)
{
    user_param_init(ctx, &ctx->para);

    /* tx */
    strncpy(ctx->tx_video_url, "test.yuv", sizeof(ctx->tx_video_url));
    ctx->tx_video_session_cnt = 0;
    strncpy(ctx->tx_audio_url, "test.wav", sizeof(ctx->tx_audio_url));
    ctx->tx_audio_session_cnt = 0;
    strncpy(ctx->tx_anc_url, "test.txt", sizeof(ctx->tx_anc_url));
    ctx->tx_anc_session_cnt = 0;
    strncpy(ctx->tx_st22_url, "test.raw", sizeof(ctx->tx_st22_url));
    ctx->tx_st22_session_cnt = 0;
    strncpy(ctx->tx_st22p_url, "test_rfc4175.yuv", sizeof(ctx->tx_st22p_url));
    ctx->tx_st22p_session_cnt = 0;
    strncpy(ctx->tx_st20p_url, "test_rfc4175.yuv", sizeof(ctx->tx_st20p_url));
    ctx->tx_st20p_session_cnt = 0;

    /* rx */
    ctx->rx_video_session_cnt = 0;
    ctx->rx_audio_session_cnt = 0;
    ctx->rx_anc_session_cnt = 0;
    ctx->rx_st22_session_cnt = 0;
    ctx->rx_st22p_session_cnt = 0;
    ctx->rx_st20p_session_cnt = 0;
    ctx->rx_max_width = 1920;
    ctx->rx_max_height = 1080;

    /* st22 */
    // ctx->st22_bpp = 3; /* 3bit per pixel */

    ctx->utc_offset = UTC_OFFSSET;

    /* init lcores and sch */
    for(int i = 0; i < ST_APP_MAX_LCORES; i++)
    {
        ctx->lcore[i] = -1;
        ctx->rtp_lcore[i] = -1;
    }
}

// initialize video source 
static int st_tx_video_source_init(struct st_app_context* ctx)
{
    st_json_context_t* c = ctx->json_ctx;
    try
    {
        for(int i = 0; i < c->tx_source_cnt; i++)
        {
            st_app_tx_source* s = &c->tx_sources[i];
            if(s->type == "decklink")
            {
                auto format_desc = seeder::core::video_format_desc(s->video_format);
                auto decklink = std::make_shared<seeder::decklink::decklink_input>(s->device_id, format_desc);
                ctx->tx_sources[i] = decklink;
                ctx->source_info[i] = s;
            }
            else if(s->type == "file")
            {
                auto format_desc = seeder::core::video_format_desc(s->video_format);
                auto ffmpeg = std::make_shared<seeder::ffmpeg::ffmpeg_input>(s->file_url,format_desc);
                ctx->tx_sources[i] = ffmpeg;
                ctx->source_info[i] = s;
            }
        }
    }
    catch(const std::exception& e)
    {
        return -1;
    }
    
    return 0;
}

static int st_tx_video_source_start(struct st_app_context* ctx)
{
    try
    {
        for(auto s : ctx->tx_sources)
        {
            if(s) s->start();        
        }
    }
    catch(const std::exception& e)
    {
        return -1;
    }

    return 0;
}

static void st_app_ctx_free(struct st_app_context* ctx)
{
    st_app_tx_video_sessions_uinit(ctx);
    st_app_tx_audio_sessions_uinit(ctx);

    if(ctx->runtime_session)
        if(ctx->st) st_stop(ctx->st);
    
    if(ctx->json_ctx)
    {
        st_app_free_json(ctx->json_ctx);
        st_app_free(ctx->json_ctx);
    }

    if(ctx->st) 
    {
        for(int i = 0; i < ST_APP_MAX_LCORES; i++) 
        {
            if(ctx->lcore[i] >= 0) {
                st_put_lcore(ctx->st, ctx->lcore[i]);
                ctx->lcore[i] = -1;
            }
            if(ctx->rtp_lcore[i] >= 0) {
                st_put_lcore(ctx->st, ctx->rtp_lcore[i]);
                ctx->rtp_lcore[i] = -1;
            }
        }
        st_uninit(ctx->st);
        ctx->st = NULL;
    }

    st_app_free(ctx);
}

static void st_app_sig_handler(int signo) {
    struct st_app_context* ctx = g_app_ctx;

    logger->info("%s, signal %d\n", __func__, signo);
    switch (signo) {
        case SIGINT: /* Interrupt from keyboard */
            if (ctx->st) st_request_exit(ctx->st);
                ctx->stop = true;
            break;
    }

    return;
}

int main(int argc, char *argv[])
{
    int ret = 0;

    // seeder::config config;
    try
    {
        // create logger
        logger = create_logger("info", "warning", "./log/log.txt", 10, 30);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return 0;
    }

    // initialize app context
    struct st_app_context *ctx;
    ctx = (st_app_context *)st_app_zmalloc(sizeof(*ctx));
    if(!ctx)
    {
        logger->error("{}, app_context alloc fail", __func__);
        return 0;
    }

    st_app_ctx_init(ctx);
    ret = st_app_parse_args(ctx, &ctx->para, argc, argv);
    if(ret < 0)
    {
        logger->error("{}, parse arguments fail", __func__);
        st_app_ctx_free(ctx);
        return ret;
    }

    if(ctx->tx_video_session_cnt > ST_APP_MAX_TX_VIDEO_SESSIONS ||
        ctx->tx_st22_session_cnt > ST_APP_MAX_TX_VIDEO_SESSIONS ||
        ctx->tx_st22p_session_cnt > ST_APP_MAX_TX_VIDEO_SESSIONS ||
        ctx->tx_st20p_session_cnt > ST_APP_MAX_TX_VIDEO_SESSIONS ||
        ctx->tx_audio_session_cnt > ST_APP_MAX_TX_AUDIO_SESSIONS ||
        ctx->tx_anc_session_cnt > ST_APP_MAX_TX_ANC_SESSIONS ||
        ctx->rx_video_session_cnt > ST_APP_MAX_RX_VIDEO_SESSIONS ||
        ctx->rx_st22_session_cnt > ST_APP_MAX_RX_VIDEO_SESSIONS ||
        ctx->rx_st22p_session_cnt > ST_APP_MAX_RX_VIDEO_SESSIONS ||
        ctx->rx_st20p_session_cnt > ST_APP_MAX_RX_VIDEO_SESSIONS ||
        ctx->rx_audio_session_cnt > ST_APP_MAX_RX_AUDIO_SESSIONS ||
        ctx->rx_anc_session_cnt > ST_APP_MAX_RX_ANC_SESSIONS)
    {
        logger->error("{}, session count invalid, pass the restriction {}", __func__,
                      ST_APP_MAX_RX_VIDEO_SESSIONS);
        st_app_ctx_free(ctx);
        return -EINVAL;
    }
    ctx->para.tx_sessions_cnt_max = ctx->tx_video_session_cnt + ctx->tx_audio_session_cnt +
                                    ctx->tx_anc_session_cnt + ctx->tx_st22_session_cnt +
                                    ctx->tx_st20p_session_cnt + ctx->tx_st22p_session_cnt;
    ctx->para.rx_sessions_cnt_max = ctx->rx_video_session_cnt + ctx->rx_audio_session_cnt +
                                    ctx->rx_anc_session_cnt + ctx->rx_st22_session_cnt +
                                    ctx->rx_st22p_session_cnt + ctx->rx_st20p_session_cnt;

    for(int i = 0; i < ctx->para.num_ports; i++)
    {
        ctx->para.pmd[i] = st_pmd_by_port_name(ctx->para.port[i]);
        if(ctx->para.tx_sessions_cnt_max > ctx->para.rx_sessions_cnt_max)
            ctx->para.xdp_info[i].queue_count = ctx->para.tx_sessions_cnt_max;
        else
            ctx->para.xdp_info[i].queue_count = ctx->para.rx_sessions_cnt_max;
    }

    if(ctx->enable_hdr_split)
    {
        ctx->para.nb_rx_hdr_split_queues = ctx->rx_video_session_cnt;
    }

    ctx->st = st_init(&ctx->para);
    if(!ctx->st)
    {
        logger->error("{}, st_init fail", __func__);
        st_app_ctx_free(ctx);
        return -ENOMEM;
    }
    
    g_app_ctx = ctx;
    if(signal(SIGINT, st_app_sig_handler) == SIG_ERR) {
        logger->error("{}, cat SIGINT fail", __func__);
        st_app_ctx_free(ctx);
        return -EIO;
    }

    // tx video
    ret = st_app_tx_video_sessions_init(ctx);
    if(ret < 0)
    {
        logger->error("{}, st_tx_video_session_init fail", __func__);
        st_app_ctx_free(ctx);
        return -EIO;
    }

    // tx audio
    ret = st_app_tx_audio_sessions_init(ctx);
    if(ret < 0)
    {
        logger->error("{}, st_tx_audio_session_init fail", __func__);
        st_app_ctx_free(ctx);
        return -EIO;
    }


    // init tx video source
    ret = st_tx_video_source_init(ctx);
    if(ret < 0)
    {
        logger->error("{}, st_tx_video_source_init fail", __func__);
        st_app_ctx_free(ctx);
        return -EIO;
    }

    if(!ctx->runtime_session)
    {
        ret = st_start(ctx->st);
        if(ret < 0 )
        {
            logger->error("{}, start device fail", __func__);
            st_app_ctx_free(ctx);
            return -EIO;
        }
    }

    // start video source input stream
    ret = st_tx_video_source_start(ctx);
    {
        logger->error("{}, st_tx_video_source_start fail", __func__);
        st_app_ctx_free(ctx);
        return -EIO;
    }

    // wait for stop
    int test_time_s = ctx->test_time_s;
    logger->debug("{}, app lunch success, test time {}", __func__, test_time_s);
    while(!ctx->stop) {
        sleep(1);
    }
    logger->debug("{}, start to ending", __func__);

    if(!ctx->runtime_session) {
        /* stop st first */
        if(ctx->st) st_stop(ctx->st);
    }

    //ret = st_app_result(ctx);

    /* free */
    st_app_ctx_free(ctx);
    return ret;
}