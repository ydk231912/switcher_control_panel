
#include <cstdint>
#include <exception>
#include <memory>

#include "tx_video.h"
#include "core/util/logger.h"
#include "core/stream/input.h"
#include "core/frame/frame.h"
#include "core/util/timer.h"
#include "core/util/timer.h"
// #include "ffmpeg/input/ffmpeg_input.h"
#include "decklink/input/decklink_input.h"


using namespace seeder::core;

void st_app_tx_video_session_reset_stat(struct st_app_tx_video_session* s) {
    s->next_frame_stat = 0;
    s->next_frame_not_ready_stat = 0;
    s->frame_done_stat = 0;
    s->build_frame_stat = 0;
}

static int app_tx_video_next_frame(void* priv, uint16_t* next_frame_idx, struct st20_tx_frame_meta* meta) 
{
    struct st_app_tx_video_session* s = (st_app_tx_video_session*)priv;
    int ret;
    uint16_t consumer_idx = s->framebuff_consumer_idx;
    struct st_tx_frame* framebuff = &s->framebuffs[consumer_idx];

    st_pthread_mutex_lock(&s->st20_wake_mutex);
    s->next_frame_stat++;
    if(ST_TX_FRAME_READY == framebuff->stat) {
        logger->trace("{}({}), next frame idx {}", __func__, s->idx, consumer_idx);
        ret = 0;
        framebuff->stat = ST_TX_FRAME_IN_TRANSMITTING;
        *next_frame_idx = consumer_idx;
        meta->second_field = framebuff->second_field;
        /* point to next */
        consumer_idx++;
        if(consumer_idx >= s->framebuff_cnt) consumer_idx = 0;
        s->framebuff_consumer_idx = consumer_idx;
    } else  {
        s->next_frame_not_ready_stat++;
        /* not ready */
        ret = -EIO;
        logger->trace("{}({}), idx {} err stat {}", __func__, s->idx, consumer_idx, framebuff->stat);
    }
    st_pthread_cond_signal(&s->st20_wake_cond);
    st_pthread_mutex_unlock(&s->st20_wake_mutex);

    return ret;
}

static int app_tx_video_frame_done(void* priv, uint16_t frame_idx, struct st20_tx_frame_meta* meta)
{
    struct st_app_tx_video_session* s = (st_app_tx_video_session*)priv;
    int ret;
    struct st_tx_frame* framebuff = &s->framebuffs[frame_idx];

    st_pthread_mutex_lock(&s->st20_wake_mutex);
    if(ST_TX_FRAME_IN_TRANSMITTING == framebuff->stat)
    {
        ret = 0;
        framebuff->stat = ST_TX_FRAME_FREE;
        logger->trace("{}({}), done_idx {}", __func__, s->idx, frame_idx);
    }else
    {
        ret = -EIO;
        logger->error("{}({}), err status {} for frame {}", __func__, s->idx, framebuff->stat,
            frame_idx);
    }
    st_pthread_cond_signal(&s->st20_wake_cond);
    st_pthread_mutex_unlock(&s->st20_wake_mutex);

    s->st20_frame_done_cnt++;
    s->frame_done_stat++;
    if(!s->stat_frame_frist_tx_time)
        s->stat_frame_frist_tx_time = st_app_get_monotonic_time();

    return ret;
}

static void app_tx_video_build_frame(struct st_app_tx_video_session* s, void* frame, size_t frame_size, struct st_tx_frame* framebuff) 
{
    int ret = 0;
    auto f = s->tx_source->get_video_frame();
    if(!f) return;


    // // convert pixel format
    if(s->source_info->type == "decklink")
    {
        if (s->interlaced) {
            if (!s->half_height_buffer) {
                s->half_height_buffer.reset(new uint8_t[f->linesize[0] * s->height]);
            }
            bool top_field_first = f->top_field_first;
            std::size_t first_line = top_field_first ? 0 : 1;
            bool second_field = f->opaque;
            framebuff->second_field = second_field;
            if (second_field) {
                first_line = 1 - first_line;
            }
            for (int i = 0; i < s->height; i++) {
                memcpy(s->half_height_buffer.get() + i * f->linesize[0], f->data[0] + (i * 2 + first_line) * f->linesize[0], f->linesize[0]);
            }
            ret = st20_v210_to_rfc4175_422be10(s->half_height_buffer.get(), (st20_rfc4175_422_10_pg2_be*)frame, s->width, s->height);
        } else {
            // convert v210 to yuv10be
            ret = st20_v210_to_rfc4175_422be10(f->data[0], (st20_rfc4175_422_10_pg2_be*)frame, s->width, s->height);
        }
        if(ret < 0)
        {
            logger->error("{}, convet v210 to yuv422be10 fail", __func__);
            return;
        }
    }
    else if(s->source_info->type == "file")
    {
        //timer t1;
        //video file pixel format must be AV_PIX_FMT_YUV422P10LE
        ret = st20_yuv422p10le_to_rfc4175_422be10((uint16_t*)f->data[0], (uint16_t*)f->data[1], 
                    (uint16_t*)f->data[2], (st20_rfc4175_422_10_pg2_be*)frame, s->width, s->height);
        //std::cout << "422 to st2110 1 frame : " << t1.elapsed() << std::endl;
        if(ret < 0)
        {
            logger->error("{}, convet yuvp42210le to yuv422be10 fail", __func__);
            return;
        }
    }
    s->build_frame_stat++;
    return;
}

static void app_tx_video_thread_bind(struct st_app_tx_video_session* s) 
{
    if(s->lcore != -1) 
    {
        mtl_bind_to_lcore(s->st, pthread_self(), s->lcore);
    }
}

static void app_tx_video_check_lcore(struct st_app_tx_video_session* s, bool rtp) 
{
    int sch_idx = st20_tx_get_sch_idx(s->handle);

    if(!s->ctx->app_thread && (s->handle_sch_idx != sch_idx)) 
    {
        s->handle_sch_idx = sch_idx;
        unsigned int lcore;
        int ret = st_app_video_get_lcore(s->ctx, s->handle_sch_idx, rtp, &lcore);
        if((ret >= 0) && (lcore != s->lcore)) 
        {
            s->lcore = lcore;
            app_tx_video_thread_bind(s);
            logger->debug("{}({}), bind to new lcore {}", __func__, s->idx, lcore);
        }
    }
}

static int app_tx_video_handle_free(struct st_app_tx_video_session* s) 
{
    int ret;
    int idx = s->idx;

    if(s->handle)
    {
        ret = st20_tx_free(s->handle);
        if(ret < 0) logger->error("{}({}), st20_tx_free fail {}", __func__, idx, ret);
        s->handle = NULL;
    }

    return 0;
}

int st_app_tx_video_session_uinit(struct st_app_tx_video_session* s) 
{
    s->st20_app_thread_stop = true;
    if(s->st20_app_thread)
    {
        /* wake up the thread */
        st_pthread_mutex_lock(&s->st20_wake_mutex);
        st_pthread_cond_signal(&s->st20_wake_cond);
        st_pthread_mutex_unlock(&s->st20_wake_mutex);
        logger->info("{}({}), wait app thread stop", __func__, s->idx);
        pthread_join(s->st20_app_thread, NULL);
    }

    app_tx_video_handle_free(s);

    if(s->framebuffs) 
    {
        st_app_free(s->framebuffs);
        s->framebuffs = NULL;
    }

    st_pthread_mutex_destroy(&s->st20_wake_mutex);
    st_pthread_cond_destroy(&s->st20_wake_cond);

    return 0;
}

static int app_tx_video_result(struct st_app_tx_video_session* s) 
{
    int idx = s->idx;
    uint64_t cur_time_ns = st_app_get_monotonic_time();
    double time_sec = (double)(cur_time_ns - s->stat_frame_frist_tx_time) / NS_PER_S;
    double framerate = s->st20_frame_done_cnt / time_sec;

    if(!s->st20_frame_done_cnt) return -EINVAL;

    logger->debug("{}({}), {}, fps {}, {} frames send", __func__, idx,
            ST_APP_EXPECT_NEAR(framerate, s->expect_fps, s->expect_fps * 0.05) ? "OK"
                                                                                : "FAILED",
            framerate, s->st20_frame_done_cnt);
    return 0;
}

static void* app_tx_video_frame_thread(void* arg) 
{
    struct st_app_tx_video_session* s = (st_app_tx_video_session*)arg;
    int idx = s->idx;
    uint16_t producer_idx;
    struct st_tx_frame* framebuff;

    app_tx_video_thread_bind(s);

    logger->debug("{}({}), start", __func__, idx);
    logger->debug("tx video {} framebuffer_size={} width={} height={}", idx, s->st20_frame_size, s->width, s->height);
    while(!s->st20_app_thread_stop) 
    {
        st_pthread_mutex_lock(&s->st20_wake_mutex);
        producer_idx = s->framebuff_producer_idx;
        framebuff = &s->framebuffs[producer_idx];
        if(ST_TX_FRAME_FREE != framebuff->stat) 
        {
            /* not in free */
            if(!s->st20_app_thread_stop)
                st_pthread_cond_wait(&s->st20_wake_cond, &s->st20_wake_mutex);
            st_pthread_mutex_unlock(&s->st20_wake_mutex);
            continue;
        }
        st_pthread_mutex_unlock(&s->st20_wake_mutex);

        app_tx_video_check_lcore(s, false);

        void* frame_addr = st20_tx_get_framebuffer(s->handle, producer_idx);
        if (!s->slice) 
        {
            /* interlaced use different layout? */
            app_tx_video_build_frame(s, frame_addr, s->st20_frame_size, framebuff);
        }

        st_pthread_mutex_lock(&s->st20_wake_mutex);
        framebuff->size = s->st20_frame_size;
        // framebuff->second_field = s->second_field;
        framebuff->stat = ST_TX_FRAME_READY;
        /* point to next */
        producer_idx++;
        if (producer_idx >= s->framebuff_cnt) producer_idx = 0;
        s->framebuff_producer_idx = producer_idx;
        if (s->interlaced) {
            s->second_field = !s->second_field;
        }
        st_pthread_mutex_unlock(&s->st20_wake_mutex);

        if(s->slice) 
        {
            //app_tx_video_build_slice(s, framebuff, frame_addr);
        }
    }
    logger->info("{}({}), stop", __func__, idx);

    return NULL;
}

static int app_tx_video_init(struct st_app_context* ctx, st_json_video_session_t* video,
                             struct st_app_tx_video_session* s) 
{
    int idx = s->idx, ret;
    struct st20_tx_ops ops;
    char name[32];
    st20_tx_handle handle;
    memset(&ops, 0, sizeof(ops));

    s->ctx = ctx;

    // tx video source
    s->tx_source = ctx->tx_sources[video->base.id];
    s->tx_source_id = video->base.id;
    s->source_info = std::make_shared<st_app_tx_source>(ctx->source_info[video->base.id]);

    snprintf(name, 32, "TX Video SDI %d", s->source_info->device_id);
    ops.name = name;
    ops.priv = s;
    ops.num_port = video ? video->base.num_inf : ctx->para.num_ports;
    memcpy(ops.dip_addr[MTL_PORT_P],
            video ? video->base.ip[MTL_PORT_P] : ctx->tx_dip_addr[MTL_PORT_P], MTL_IP_ADDR_LEN);
    strncpy(ops.port[MTL_PORT_P],
            video ? video->base.inf[MTL_PORT_P].name : ctx->para.port[MTL_PORT_P], MTL_PORT_MAX_LEN);
    ops.udp_port[MTL_PORT_P] = video ? video->base.udp_port : (10000 + s->idx);
    if(ctx->has_tx_dst_mac[MTL_PORT_P]) 
    {
        memcpy(&ops.tx_dst_mac[MTL_PORT_P][0], ctx->tx_dst_mac[MTL_PORT_P], 6);
        ops.flags |= ST20_TX_FLAG_USER_P_MAC;
    }
    if(ops.num_port > 1)
    {
        memcpy(ops.dip_addr[MTL_PORT_R],
            video ? video->base.ip[MTL_PORT_R] : ctx->tx_dip_addr[MTL_PORT_R], MTL_IP_ADDR_LEN);
        strncpy(ops.port[MTL_PORT_R],
                video ? video->base.inf[MTL_PORT_R].name : ctx->para.port[MTL_PORT_R], MTL_PORT_MAX_LEN);
        ops.udp_port[MTL_PORT_R] = video ? video->base.udp_port : (10000 + s->idx);
        if(ctx->has_tx_dst_mac[MTL_PORT_R]) 
        {
            memcpy(&ops.tx_dst_mac[MTL_PORT_R][0], ctx->tx_dst_mac[MTL_PORT_R], 6);
            ops.flags |= ST20_TX_FLAG_USER_R_MAC;
        }
    }
    ops.pacing = ST21_PACING_NARROW;
    ops.packing = video ? video->info.packing : ST20_PACKING_BPM;
    ops.type = video ? video->info.type : ST20_TYPE_FRAME_LEVEL;
    ops.width = video ? st_app_get_width(video->info.video_format) : 1920;
    ops.height = video ? st_app_get_height(video->info.video_format) : 1080;
    ops.fps = video ? st_app_get_fps(video->info.video_format) : ST_FPS_P59_94;
    ops.fmt = video ? video->info.pg_format : ST20_FMT_YUV_422_10BIT;
    ops.interlaced = video ? st_app_get_interlaced(video->info.video_format) : false;
    ops.get_next_frame = app_tx_video_next_frame;
    ops.notify_frame_done = app_tx_video_frame_done;
    // ops.query_frame_lines_ready = app_tx_video_frame_lines_ready;
    // ops.notify_rtp_done = app_tx_video_rtp_done;
    ops.framebuff_cnt = 2;
    ops.payload_type = video ? video->base.payload_type : ST_APP_PAYLOAD_TYPE_VIDEO;

    ret = st20_get_pgroup(ops.fmt, &s->st20_pg);
    if (ret < 0) return ret;

    s->width = ops.width;
    s->full_height = s->height = ops.height;
    s->interlaced = ops.interlaced ? true : false;
    if (s->interlaced) s->height >>= 1;
    s->linesize = s->width * s->st20_pg.size / s->st20_pg.coverage;
    memcpy(s->st20_source_url, video ? video->info.video_url : ctx->tx_video_url,
            ST_APP_URL_MAX_LEN);
    s->st20_pcap_input = false;
    s->st20_rtp_input = false;
    s->st = ctx->st;
    s->single_line = (ops.packing == ST20_PACKING_GPM_SL);
    s->slice = (ops.type == ST20_TYPE_SLICE_LEVEL);
    s->expect_fps = st_frame_rate(ops.fps);
    s->payload_type = ops.payload_type;

    s->framebuff_cnt = ops.framebuff_cnt;
    s->lines_per_slice = ops.height / 30;
    s->st20_source_fd = -1;

    s->framebuffs =
        (struct st_tx_frame*)st_app_zmalloc(sizeof(*s->framebuffs) * s->framebuff_cnt);
    if(!s->framebuffs) 
    {
        return -ENOMEM;
    }
    for(uint16_t j = 0; j < s->framebuff_cnt; j++) 
    {
        s->framebuffs[j].stat = ST_TX_FRAME_FREE;
        s->framebuffs[j].lines_ready = 0;
    }

    st_pthread_mutex_init(&s->st20_wake_mutex, NULL);
    st_pthread_cond_init(&s->st20_wake_cond, NULL);

    handle = st20_tx_create(ctx->st, &ops);
    if(!handle) 
    {
        logger->error("{}({}), st20_tx_create fail", __func__, idx);
        st_app_tx_video_session_uinit(s);
        return -EIO;
    }
    s->handle = handle;
    s->st20_frame_size = st20_tx_get_framebuffer_size(handle);
    s->handle_sch_idx = st20_tx_get_sch_idx(handle);
    unsigned int lcore;
    bool rtp = false;
    if (ops.type == ST20_TYPE_RTP_LEVEL) rtp = true;

    if(!ctx->app_thread) 
    {
        ret = st_app_video_get_lcore(ctx, s->handle_sch_idx, rtp, &lcore);
        if (ret >= 0) s->lcore = lcore;
    }

    // start frame thread to build frame
    ret = pthread_create(&s->st20_app_thread, NULL, app_tx_video_frame_thread, s);
    if(ret < 0)
    {
        logger->error("{}, st20_app_thread create fail, err = {}", __func__, ret);
        st_app_tx_video_session_uinit(s);
        return -EIO;
    }

    return 0;
}

static int st_app_tx_video_session_init(st_app_context* ctx, st_json_video_session_t *json_video, st_app_tx_video_session *s) {
    int ret = 0;
    s->idx = ctx->next_tx_video_session_idx++;
    s->lcore = -1;
    ret = app_tx_video_init(ctx, json_video, s);
    if (ret < 0) logger->error("{}({}), app_tx_video_init fail {}", __func__, s->idx, ret);
    return ret;
}

int st_app_tx_video_sessions_init(struct st_app_context* ctx) 
{
    int ret = 0;
    for (auto &json_video : ctx->json_ctx->tx_video_sessions) {
        if (!json_video.base.enable) {
            continue;
        }
        auto s = std::shared_ptr<st_app_tx_video_session>(new st_app_tx_video_session {});
        ret = st_app_tx_video_session_init(ctx, &json_video, s.get());
        if (ret) {
            continue;
        }
        ctx->tx_video_sessions.push_back(s);
    }
    return 0;
}

int st_app_tx_video_sessions_add(st_app_context* ctx, st_json_context_t *new_json_ctx) {
    int ret = 0;
    for (auto &json_video : new_json_ctx->tx_video_sessions) {
        if (!json_video.base.enable) {
            continue;
        }
        auto s = std::shared_ptr<st_app_tx_video_session>(new st_app_tx_video_session {});
        ret = st_app_tx_video_session_init(ctx, &json_video, s.get());
        if (ret) {
            continue;
        }
        ctx->tx_video_sessions.push_back(s);
    }
    return 0;
}

int st_tx_video_source_init(struct st_app_context* ctx, st_json_context_t *c) {
    for(int i = 0; i < c->tx_sources.size(); i++) {
        auto &s = c->tx_sources[i];
        auto &info = c->tx_audio_sessions[i].info;
        auto format_desc = seeder::core::video_format_desc::get(s.video_format);
        try {
            if(s.type == "decklink") {
                
                st_set_video_foramt(info, &format_desc);
                auto decklink = std::make_shared<seeder::decklink::decklink_input>(s.id, s.device_id, format_desc);
                ctx->tx_sources.emplace(decklink->get_source_id(), decklink);
                ctx->source_info.emplace(decklink->get_source_id(), s);
            }
            //  else if(s.type == "file") {
            //     auto ffmpeg = std::make_shared<seeder::ffmpeg::ffmpeg_input>(s.id, s.file_url, format_desc);
            //     ctx->tx_sources.emplace(ffmpeg->get_source_id(), ffmpeg);
            //     ctx->source_info.emplace(ffmpeg->get_source_id(), s);
            // }
        } catch (std::exception &e) {
            logger->info("tx_video_source_init failed {} device_id={} video_format={}", e.what(), s.device_id, s.video_format);
            return -1;
        }
    }
    
    return 0;
}

int st_tx_video_source_start(st_app_context *ctx, seeder::core::input *s) {
    try {
        if (s) s->start();
    } catch(const std::exception& e) {
        return -1;
    }
    return 0;
}


int st_app_tx_video_sessions_uinit(struct st_app_context* ctx) 
{
    for (auto &s : ctx->tx_video_sessions) {
        if (s) {
            st_app_tx_video_session_uinit(s.get());
        }
    }
    ctx->tx_video_sessions.clear();

    return 0;
}

int st_app_tx_video_sessions_result(struct st_app_context* ctx) 
{
    int ret = 0;
    for (auto &s : ctx->tx_video_sessions) {
        ret += app_tx_video_result(s.get());
    }
    return ret;
}
