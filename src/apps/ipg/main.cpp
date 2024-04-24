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

#include <json/value.h>
#include <json/writer.h>
#include <string>
#include <future>
#include <exception>
#include <iostream>
#include <malloc.h>
#include <functional>

#include "core/util/logger.h"
#include "core/video_format.h"
#include "decklink/input/decklink_input.h"
#include "decklink/manager.h"
#include "decklink/output/decklink_output.h"
// #include "ffmpeg/input/ffmpeg_input.h"


#include "args.h"

#include "app_base.h"
#include "tx_video.h"
#include "tx_audio.h"
#include "rx_video.h"
#include "rx_audio.h"
#include "http_server.h"
#include "app_stat.h"
#include <mtl/mtl_seeder_api.h>

extern "C"
{
    #include "test.h"
}

// #include "player.h"
#include <rte_lcore.h>

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
    st_app_update_stat(ctx);
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
    p->dump_period_s = 10;
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


static int st_tx_video_source_start(struct st_app_context* ctx)
{
    for(auto &[id, s] : ctx->tx_sources) {
        st_tx_video_source_start(ctx, s.get());
    }
    return 0;
} 

static int st_rx_output_start(struct st_app_context* ctx)
{
    for (auto &[id, s] : ctx->rx_output) {
        st_rx_output_start(ctx, s.get());
    }
    return 0;
}

static int st_sdi_output_start(struct st_app_context* ctx)
{
    for(int i = 0; i < ctx->tx_video_sessions.size(); i++)
    {
        for(auto &[source_id,outputsdi] : ctx->tx_video_sessions[i]->sdi_output)
        {
            st_tx_sdi_output_start(ctx,outputsdi.get());
        }
    }

    return 0;
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

namespace {


class JsonObjectDeleter {
public:
  void operator()(json_object *o) {
    json_object_put(o);
  }
};

std::unique_ptr<json_object, JsonObjectDeleter> convert_json_object(const Json::Value &v) {
    Json::FastWriter writer;
    auto s = writer.write(v);
    return std::unique_ptr<json_object, JsonObjectDeleter>(json_tokener_parse(s.c_str()));
}


} // namespace

// check args are correct
int st_app_args_check(struct st_app_context *ctx)
{
    if (!ctx->json_ctx) {
        return -1;
    }
    int max_tx_queue_count = 16 * 4; // 16 路 * 2 (video,audio) * 2 (冗余)
    int max_rx_queue_count = 16 * 4;
#if MTL_VERSION_MAJOR < 23
    ctx->para.tx_sessions_cnt_max = max_tx_queue_count;
    ctx->para.rx_sessions_cnt_max = max_rx_queue_count;
#endif
    for(int i = 0; i < ctx->para.num_ports; i++)
    {
        ctx->para.pmd[i] = mtl_pmd_by_port_name(ctx->para.port[i]);
#if MTL_VERSION_MAJOR >= 23
        ctx->para.tx_queues_cnt[i] = max_tx_queue_count;
        ctx->para.rx_queues_cnt[i] = max_rx_queue_count;
#else
        ctx.params.xdp_info[i].queue_count = max_tx_queue_count + max_rx_queue_count;
#endif
    }

    if(ctx->enable_hdr_split)
    {
        ctx->para.nb_rx_hdr_split_queues = ctx->rx_video_sessions.size();
    }

    auto &ptp_config = ctx->json_ctx->json_root["ptp_config"];
    if (ptp_config.isObject()) {
        auto ptp_config_obj = convert_json_object(ptp_config);
        if (ptp_config_obj && mtl_seeder_set_ptp_params(ptp_config_obj.get())) {
            logger->error("mtl_seeder_set_ptp_params failed");
            return -1;
        }
    }

    return 0;
}

static void init_decklink(st_app_context *ctx) {
    auto &manager = seeder::decklink::device_manager::instance();
    for (auto &device : manager.get_device_status()) {
        logger->info(
            "decklink index={} display_name={} persistent_id={} device_label={} support_format_dection={}",
            device.index, device.display_name, device.persistent_id, device.device_label, device.support_format_dection
        );
    }
    const auto &devices = ctx->json_ctx->json_root["decklink"]["devices"];
    if (devices.isArray()) {
        std::vector<int> id_map;
        for (Json::Value::ArrayIndex i = 0; i < devices.size(); ++i) {
            id_map.push_back(devices[i]["id"].asInt());
        }
        manager.set_id_map(id_map);
    } else {
        manager.set_id_map(ctx->decklink_id_map);
    }
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
        st_app_init_logger_from_args(argc, argv);
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

    init_decklink(ctx.get());

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

    //output_sdi
    ret = st_app_output_sdi_init(ctx.get(), ctx->json_ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_app_output_sdi_init fail", __func__);
        return -EIO;
    }

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

    //start video source sdi output stream
    ret = st_sdi_output_start(ctx.get());
    if(ret < 0)
    {
        logger->error("{}, st_sdi_output_start fail", __func__);
        return -EIO;
    }

    logger->info("lcore count={}", rte_lcore_count());

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



