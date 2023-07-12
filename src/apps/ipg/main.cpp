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
#include "decklink/output/decklink_output.h"
#include "ffmpeg/input/ffmpeg_input.h"


#include "args.h"

#include "app_base.h"
#include "tx_video.h"
#include "tx_audio.h"
#include "rx_video.h"
#include "rx_audio.h"
#include "http_server.h"

extern "C"
{
    #include "test.h"
}

#include "player.h"

// #include "core/video_format.h"
// #include "config.h"
// #include "server.h"

using namespace seeder::core;

static struct st_app_context* g_app_ctx; /* only for st_app_sig_handler */
static enum mtl_log_level app_log_level;

void app_set_log_level(enum mtl_log_level level) { app_log_level = level; }

enum mtl_log_level app_get_log_level(void) { return app_log_level; }

static void app_stat(void *priv)
{
    struct st_app_context *ctx = (st_app_context *)priv;
    std::lock_guard<std::mutex> lock_guard(ctx->mutex);
    
    st_app_rx_video_sessions_stat(ctx);
    st_app_rx_output_stat(ctx);
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

int st_app_video_get_lcore(struct st_app_context* ctx, int sch_idx, bool rtp, unsigned int* lcore)
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
            ret = mtl_get_lcore(ctx->st, &video_lcore);
            if(ret < 0) return ret;
            ctx->rtp_lcore[sch_idx] = video_lcore;
            logger->info("{}, new rtp lcore {} for sch idx {}", __func__, video_lcore, sch_idx);
        }
    }
    else 
    {
        if(ctx->lcore[sch_idx] < 0) 
        {
            ret = mtl_get_lcore(ctx->st, &video_lcore);
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

static void user_param_init(struct st_app_context *ctx, struct mtl_init_params *p)
{
    memset(p, 0x0, sizeof(*p));

    p->pmd[MTL_PORT_P] = MTL_PMD_DPDK_USER;
    p->pmd[MTL_PORT_R] = MTL_PMD_DPDK_USER;
    /* defalut start queue set to 1 */
    p->xdp_info[MTL_PORT_P].start_queue = 1;
    p->xdp_info[MTL_PORT_R].start_queue = 1;
    p->flags |= MTL_FLAG_BIND_NUMA; /* default bind to numa */
    p->flags |= MTL_FLAG_TX_VIDEO_MIGRATE;
    p->flags |= MTL_FLAG_RX_VIDEO_MIGRATE;
    p->flags |= MTL_FLAG_RX_SEPARATE_VIDEO_LCORE;
    p->priv = ctx;
    p->ptp_get_time_fn = app_ptp_from_tai_time;
    p->stat_dump_cb_fn = app_stat;
    p->log_level = MTL_LOG_LEVEL_INFO;//ST_LOG_LEVEL_INFO;
    //p->log_level = ST_LOG_LEVEL_INFO;//ST_LOG_LEVEL_INFO;
    // app_set_log_level(p->log_level);
}

static void st_app_ctx_init(struct st_app_context *ctx)
{
    user_param_init(ctx, &ctx->para);

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

/*
static int st_tx_video_source_init_update(struct st_app_context* ctx,st_json_context_t* c,int id)
{
    try
    {
        for(int i = 0; i < c->tx_source_cnt; i++)
        {
            if (id == c->tx_video_sessions[i].tx_source_id)
            {
                struct st_app_tx_source* s = &c->tx_sources[i];
                if(s->type == "decklink")
                {
                    auto format_desc = seeder::core::video_format_desc(s->video_format);
                    auto info = c->tx_audio_sessions[i].info;
                    set_video_foramt(info, &format_desc);
                    auto decklink = std::make_shared<seeder::decklink::decklink_input>(s->device_id, format_desc);
                    ctx->tx_sources[id] =decklink;
                    ctx->source_info[id] = s;
                }
                else if(s->type == "file")
                {
                    auto format_desc = seeder::core::video_format_desc(s->video_format);
                    auto ffmpeg = std::make_shared<seeder::ffmpeg::ffmpeg_input>(s->file_url, format_desc);
                    ctx->tx_sources[id] =ffmpeg ;
                    ctx->source_info[id] = s;
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        return -1;
    } 
    return 0;
}
*/

/*
static int st_tx_video_source_init_add(struct st_app_context* ctx,st_json_context_t* c)
{
    try
    {
        for(int i = 0; i < c->tx_source_cnt; i++)
        {
            struct st_app_tx_source* s = &c->tx_sources[i];
            if(s->type == "decklink")
            {
                auto format_desc = seeder::core::video_format_desc(s->video_format);
                auto info = c->tx_audio_sessions[i].info;
                set_video_foramt(info, &format_desc);
                auto decklink = std::make_shared<seeder::decklink::decklink_input>(s->device_id, format_desc);
                ctx->tx_sources.push_back(decklink);
                ctx->source_info.push_back(s);
            }
            else if(s->type == "file")
            {
                auto format_desc = seeder::core::video_format_desc(s->video_format);
                auto ffmpeg = std::make_shared<seeder::ffmpeg::ffmpeg_input>(s->file_url, format_desc);
                ctx->tx_sources.push_back(ffmpeg);
                ctx->source_info.push_back(s);
            }
        }
    }
    catch(const std::exception& e)
    {
        return -1;
    }
    return 0;
}
*/



static int st_tx_video_source_start(struct st_app_context* ctx)
{
    for(auto &[id, s] : ctx->tx_sources) {
        st_tx_video_source_start(ctx, s.get());
    }
    return 0;
} 

/*


// initialize video output 
static int st_rx_output_init_add(struct st_app_context* ctx,st_json_context_t* c)
{
    try
    {
        for(int i = 0; i < c->rx_output_cnt; i++)
        {
            st_app_rx_output* s = &c->rx_output[i];
            auto info = c->rx_audio_sessions[i].info;
            auto session = c->rx_audio_sessions[i];
            if(s->type == "decklink")
            {
                auto desc = seeder::core::video_format_desc(s->video_format);
                set_video_foramt(info, &desc);

                auto decklink = std::make_shared<seeder::decklink::decklink_output>(s->device_id, desc);
                ctx->rx_output.push_back(decklink);
                ctx->output_info.push_back(s);
            
                //  ctx->rx_video_sessions[i].rx_output = decklink;
                //  ctx->rx_video_sessions[i].output_info = s;
                // ctx->rx_audio_sessions[i].rx_output = decklink;
                // ctx->rx_audio_sessions[i].output_info = s;
            }
            else if(s->type == "file")
            {
                 auto format_desc = seeder::core::video_format_desc(s->video_format);
                 auto ffmpeg = std::make_shared<seeder::ffmpeg::ffmpeg_input>(s->file_url, format_desc);
                //   ctx->rx_output.push_back(ffmpeg);
                //   ctx->output_info.push_back(s);
            }
        }
    }
    catch(const std::exception& e)
    {
        return -1;
    }
    
    return 0;
}


static int st_rx_output_init_update(struct st_app_context* ctx,st_json_context_t* c,int id)
{
    try
    {
        for(int i = 0; i < c->rx_output_cnt; i++)
        {
            st_app_rx_output* s = &c->rx_output[i];
             auto info = c->rx_audio_sessions[i].info;
            if(c->rx_audio_sessions[i].rx_output_id  == id)
            {
            auto session = ctx->rx_audio_sessions[i];

            if(s->type == "decklink")
            {
                auto desc = seeder::core::video_format_desc(s->video_format);
                set_video_foramt(info, &desc);

                auto decklink = std::make_shared<seeder::decklink::decklink_output>(s->device_id, desc);
                ctx->rx_output[id]= decklink;
                ctx->output_info[id] = s;
                // ctx->rx_video_sessions[i].rx_output = decklink;
                // ctx->rx_video_sessions[i].output_info = s;
                // ctx->rx_audio_sessions[i].rx_output = decklink;
                // ctx->rx_audio_sessions[i].output_info = s;
            }
            else if(s->type == "file")
            {
                // auto format_desc = seeder::core::video_format_desc(s->video_format);
                // auto ffmpeg = std::make_shared<seeder::ffmpeg::ffmpeg_input>(s->file_url,format_desc);
                // ctx->tx_sources[i] = ffmpeg;
                // ctx->source_info[i] = s;
            }
                
            }
        }
    }
    catch(const std::exception& e)
    {
        return -1;
    }
    
    return 0;
}




*/

static int st_rx_output_start(struct st_app_context* ctx)
{
    for (auto &[id, s] : ctx->rx_output) {
        st_rx_output_start(ctx, s.get());
    }
}

static void st_app_sig_handler(int signo) 
{
    struct st_app_context* ctx = g_app_ctx;

    logger->info("%s, signal %d\n", __func__, signo);
    switch (signo) {
        case SIGINT: /* Interrupt from keyboard */
            if (ctx->st) mtl_abort(ctx->st);
                ctx->stop = true;
            break;
    }

    return;
}

// check args are correct
int st_app_args_check(struct st_app_context *ctx)
{
    if(ctx->json_ctx->tx_video_sessions.size() > ST_APP_MAX_TX_VIDEO_SESSIONS ||
        ctx->json_ctx->tx_st22p_sessions.size() > ST_APP_MAX_TX_VIDEO_SESSIONS ||
        ctx->json_ctx->tx_st20p_sessions.size() > ST_APP_MAX_TX_VIDEO_SESSIONS ||
        ctx->json_ctx->tx_audio_sessions.size() > ST_APP_MAX_TX_AUDIO_SESSIONS ||
        ctx->json_ctx->tx_anc_sessions.size() > ST_APP_MAX_TX_ANC_SESSIONS ||
        ctx->json_ctx->rx_video_sessions.size() > ST_APP_MAX_RX_VIDEO_SESSIONS ||
        ctx->json_ctx->rx_st22p_sessions.size() > ST_APP_MAX_RX_VIDEO_SESSIONS ||
        ctx->json_ctx->rx_st20p_sessions.size() > ST_APP_MAX_RX_VIDEO_SESSIONS ||
        ctx->json_ctx->rx_audio_sessions.size() > ST_APP_MAX_RX_AUDIO_SESSIONS ||
        ctx->json_ctx->rx_anc_sessions.size() > ST_APP_MAX_RX_ANC_SESSIONS)
    {
        return -1;
    }
    if (ctx->para.tx_sessions_cnt_max == 0) {
        ctx->para.tx_sessions_cnt_max = ctx->json_ctx->tx_video_sessions.size() + ctx->json_ctx->tx_audio_sessions.size() +
                                        ctx->json_ctx->tx_anc_sessions.size() +
                                        ctx->json_ctx->tx_st20p_sessions.size() + ctx->json_ctx->tx_st22p_sessions.size();
    }
    if (ctx->para.rx_sessions_cnt_max == 0) {
        ctx->para.rx_sessions_cnt_max = ctx->json_ctx->rx_video_sessions.size() + ctx->json_ctx->rx_audio_sessions.size() +
                                        ctx->json_ctx->rx_anc_sessions.size() +
                                        ctx->json_ctx->rx_st22p_sessions.size() + ctx->json_ctx->rx_st20p_sessions.size();
    }
    logger->info("para.tx_sessions_cnt_max={} para.rx_sessions_cnt_max={}", ctx->para.tx_sessions_cnt_max, ctx->para.rx_sessions_cnt_max);
    

    for(int i = 0; i < ctx->para.num_ports; i++)
    {
        ctx->para.pmd[i] = mtl_pmd_by_port_name(ctx->para.port[i]);
        if(ctx->para.tx_sessions_cnt_max > ctx->para.rx_sessions_cnt_max) {
            ctx->para.xdp_info[i].queue_count = ctx->para.tx_sessions_cnt_max;
        }
        else {
            ctx->para.xdp_info[i].queue_count = ctx->para.rx_sessions_cnt_max;
        }
    }

    if(ctx->enable_hdr_split)
    {
        ctx->para.nb_rx_hdr_split_queues = ctx->rx_video_sessions.size();
    }

    return 0;
}

// initialize app context
std::unique_ptr<st_app_context> ctx;

int main(int argc, char **argv)
{

    ctx = std::unique_ptr<st_app_context>(new st_app_context {});
    int ret = 0;
    // seeder::config config;
    try
    {
        // create logger
        logger = create_logger("info", "error","./log/log.txt", 10, 30);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return 0;
    }

    if(!ctx)
    {
        logger->error("{}, app_context alloc fail", __func__);
        return 0;
    }

    st_app_ctx_init(ctx.get());
    ret = st_app_parse_args(ctx.get(), &ctx->para, argc, argv);
    if(ret < 0)
    {
        logger->error("{}, parse arguments fail", __func__);
        return ret;
    }

    ret = st_app_args_check(ctx.get());
    if(ret < 0)
    {
        logger->error("{}, session count invalid, pass the restriction {}", __func__,
                      ST_APP_MAX_RX_VIDEO_SESSIONS);
        return -EINVAL;
    }

    ctx->st = mtl_init(&ctx->para);
    if(!ctx->st)
    {
        logger->error("{}, st_init fail", __func__);
        return -ENOMEM;
    }
    
    g_app_ctx = ctx.get();
    if(signal(SIGINT, st_app_sig_handler) == SIG_ERR)
    {
        logger->error("{}, cat SIGINT fail", __func__);
        return -EIO;
    }

    //    // init rx video source
    ret = st_app_rx_output_init(ctx.get(), ctx->json_ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_rx_output_init fail", __func__);
        return -EIO;
    }


    // init tx video source
    ret = st_tx_video_source_init(ctx.get(), ctx->json_ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_tx_video_source_init fail", __func__);
        return -EIO;
    }

    // rx video
    ret = st_app_rx_video_sessions_init(ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_app_rx_video_sessions_init fail", __func__);
        return -EIO;
    }

    //rx audio
    ret =  st_app_rx_audio_sessions_init(ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_app_rx_audio_sessions_init fail", __func__);
        return -EIO;
    }


    //tx video
    ret = st_app_tx_video_sessions_init(ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_app_tx_video_session_init fail", __func__);
        return -EIO;
    }

    //tx_audio
    ret = st_app_tx_audio_sessions_init(ctx.get());

   //start video source input stream
    ret = st_tx_video_source_start(ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_tx_video_source_start fail", __func__);
        return -EIO;
    } 


    //start video source input stream
    ret = st_rx_output_start(ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_rx_output_start fail", __func__);
        return -EIO;
    }


    if(!ctx->runtime_session)
    {
        ret = mtl_start(ctx->st);
        if(ret < 0 )
        {
            logger->error("{}, start device fail", __func__);
            return -EIO;
        }
    }

    seeder::st_http_server http_server {ctx.get()};

    http_server.start();

    // wait for stop
    int test_time_s = ctx->test_time_s;
    logger->debug("{}, app lunch success, test time {}", __func__, test_time_s);
    while(!ctx->stop)
    {
        sleep(1);
    }
    logger->debug("{}, start to ending", __func__);
    http_server.stop();

    if(!ctx->runtime_session) {
        /* stop st first */
        if(ctx->st) mtl_stop(ctx->st);
    }

    //ret = st_app_result(ctx);

    return ret;
}



