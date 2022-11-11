
#include "rx_video.h"
#include "core/util/logger.h"


using namespace seeder::core;

static int app_rx_video_enqueue_frame(struct st_app_rx_video_session* s, void* frame, size_t size)
{
    uint16_t producer_idx = s->framebuff_producer_idx;
    struct st_rx_frame* framebuff = &s->framebuffs[producer_idx];

    if(framebuff->frame)
    {
        return -EBUSY;
    }

    // printf("%s(%d), frame idx %d\n", __func__, s->idx, producer_idx);
    framebuff->frame = frame;
    framebuff->size = size;
    /* point to next */
    producer_idx++;
    if(producer_idx >= s->framebuff_cnt) producer_idx = 0;
    s->framebuff_producer_idx = producer_idx;
    return 0;
}

/**
 * @brief output to decklink sdi
 * 
 * @param s 
 * @param frame 
 * @param frame_size 
 */
static void app_rx_video_consume_frame(struct st_app_rx_video_session* s, void* frame, size_t frame_size)
{
    auto output = s->rx_output;
    if(output)
    {
        // convert to v210
        st20_rfc4175_422be10_to_v210((st20_rfc4175_422_10_pg2_be*)frame, output->vframe_buffer, s->width, s->height);
        output->display_video_frame(output->vframe_buffer);
    }
}

static void* app_rx_video_frame_thread(void* arg)
{
    struct st_app_rx_video_session* s = (st_app_rx_video_session*)arg;
    int idx = s->idx;
    int consumer_idx;
    struct st_rx_frame* framebuff;

    logger->info("{}({}), start", __func__, idx);
    while(!s->st20_app_thread_stop)
    {
        st_pthread_mutex_lock(&s->st20_wake_mutex);
        consumer_idx = s->framebuff_consumer_idx;
        framebuff = &s->framebuffs[consumer_idx];
        if(!framebuff->frame)
        {
            /* no ready frame */
            if(!s->st20_app_thread_stop)
                st_pthread_cond_wait(&s->st20_wake_cond, &s->st20_wake_mutex);
            st_pthread_mutex_unlock(&s->st20_wake_mutex);
            continue;
        }
        st_pthread_mutex_unlock(&s->st20_wake_mutex);

        logger->debug("{}({}), frame idx {}", __func__, idx, consumer_idx);
        app_rx_video_consume_frame(s, framebuff->frame, framebuff->size);
        st20_rx_put_framebuff(s->handle, framebuff->frame);
        /* point to next */
        st_pthread_mutex_lock(&s->st20_wake_mutex);
        framebuff->frame = NULL;
        consumer_idx++;
        if (consumer_idx >= s->framebuff_cnt) consumer_idx = 0;
        s->framebuff_consumer_idx = consumer_idx;
        st_pthread_mutex_unlock(&s->st20_wake_mutex);
    }
    logger->info("{}({}), stop", __func__, idx);

    return NULL;
}

static int app_rx_video_init_frame_thread(struct st_app_rx_video_session* s) 
{
    int ret, idx = s->idx;

    /* user do not require fb save to file or display */
    if(s->st20_dst_fb_cnt < 1 && s->display == NULL) return 0;

    ret = pthread_create(&s->st20_app_thread, NULL, app_rx_video_frame_thread, s);
    if(ret < 0)
    {
        logger->error("{}({}), st20_app_thread create fail {}", __func__, ret, idx);
        return -EIO;
    }

    return 0;
}

static int app_rx_video_frame_ready(void* priv, void* frame, struct st20_rx_frame_meta* meta)
{
    struct st_app_rx_video_session* s = (st_app_rx_video_session*)priv;
    int ret;

    if(!s->handle) return -EIO;

    /* incomplete frame */
    if(!st_is_frame_complete(meta->status))
    {
        st20_rx_put_framebuff(s->handle, frame);
        return 0;
    }

    s->stat_frame_received++;
    if(s->measure_latency)
    {
        uint64_t latency_ns;
        uint64_t ptp_ns = st_ptp_read_time(s->st);
        uint32_t sampling_rate = 90 * 1000;

        if(meta->tfmt == ST10_TIMESTAMP_FMT_MEDIA_CLK)
        {
            uint32_t latency_media_clk = st10_tai_to_media_clk(ptp_ns, sampling_rate) - meta->timestamp;
            latency_ns = st10_media_clk_to_ns(latency_media_clk, sampling_rate);
        }
        else
        {
            latency_ns = ptp_ns - meta->timestamp;
        }
        logger->debug("{}, latency_us {}", __func__, latency_ns / 1000);
        s->stat_latency_us_sum += latency_ns / 1000;
    }
    s->stat_frame_total_received++;
    if(!s->stat_frame_frist_rx_time)
        s->stat_frame_frist_rx_time = st_app_get_monotonic_time();

    if(s->st20_dst_fd < 0 && s->display == NULL)
    {
        /* free the queue directly as no read thread is running */
        st20_rx_put_framebuff(s->handle, frame);
        return 0;
    }

    st_pthread_mutex_lock(&s->st20_wake_mutex);
    ret = app_rx_video_enqueue_frame(s, frame, meta->frame_total_size);
    if(ret < 0)
    {
        /* free the queue */
        st20_rx_put_framebuff(s->handle, frame);
        st_pthread_mutex_unlock(&s->st20_wake_mutex);
        return ret;
    }
    st_pthread_cond_signal(&s->st20_wake_cond);
    st_pthread_mutex_unlock(&s->st20_wake_mutex);

    return 0;
}

static int app_rx_video_detected(void* priv, const struct st20_detect_meta* meta,
                                 struct st20_detect_reply* reply) 
{
    struct st_app_rx_video_session* s = (st_app_rx_video_session*)priv;

    if(s->slice) reply->slice_lines = meta->height / 32;
    if(s->user_pg.fmt != USER_FMT_MAX)
    {
        int ret = user_get_pgroup(s->user_pg.fmt, &s->user_pg);
        if(ret < 0) return ret;
        reply->uframe_size = meta->width * meta->height * s->user_pg.size / s->user_pg.coverage;
    }

    return 0;
}

static int app_rx_video_uinit(struct st_app_rx_video_session* s)
{
    int ret, idx = s->idx;

    //st_app_uinit_display(s->display);
    if(s->display)
    {
        st_app_free(s->display);
    }

    if(s->framebuffs) 
    {
        st_app_free(s->framebuffs);
        s->framebuffs = NULL;
    }

    s->st20_app_thread_stop = true;
    if(s->st20_app_thread)
    {
        /* wake up the thread */
        st_pthread_mutex_lock(&s->st20_wake_mutex);
        st_pthread_cond_signal(&s->st20_wake_cond);
        st_pthread_mutex_unlock(&s->st20_wake_mutex);
        logger->info("{}({}), wait app thread stop", __func__, idx);
        pthread_join(s->st20_app_thread, NULL);
    }

    st_pthread_mutex_destroy(&s->st20_wake_mutex);
    st_pthread_cond_destroy(&s->st20_wake_cond);

    if(s->handle)
    {
        ret = st20_rx_free(s->handle);
        if(ret < 0) logger->error("{}({}), st20_rx_free fail {}", __func__, idx, ret);
        s->handle = NULL;
    }

    return 0;
}

static int app_rx_video_init(struct st_app_context* ctx, st_json_video_session_t* video,
                             struct st_app_rx_video_session* s) 
{
    int idx = s->idx, ret;
    struct st20_rx_ops ops;
    char name[32];
    st20_rx_handle handle;
    memset(&ops, 0, sizeof(ops));

    snprintf(name, 32, "app_rx_video_%d", idx);
    ops.name = name;
    ops.priv = s;
    ops.num_port = video ? video->base.num_inf : ctx->para.num_ports;
    memcpy(ops.sip_addr[ST_PORT_P],
            video ? video->base.ip[ST_PORT_P] : ctx->rx_sip_addr[ST_PORT_P], ST_IP_ADDR_LEN);
    strncpy(ops.port[ST_PORT_P],
            video ? video->base.inf[ST_PORT_P]->name : ctx->para.port[ST_PORT_P],
            ST_PORT_MAX_LEN);
    ops.udp_port[ST_PORT_P] = video ? video->base.udp_port : (10000 + s->idx);
    if(ops.num_port > 1)
    {
        memcpy(ops.sip_addr[ST_PORT_R],
                video ? video->base.ip[ST_PORT_R] : ctx->rx_sip_addr[ST_PORT_R],
                ST_IP_ADDR_LEN);
        strncpy(ops.port[ST_PORT_R],
                video ? video->base.inf[ST_PORT_R]->name : ctx->para.port[ST_PORT_R],
                ST_PORT_MAX_LEN);
        ops.udp_port[ST_PORT_R] = video ? video->base.udp_port : (10000 + s->idx);
    }
    ops.pacing = ST21_PACING_NARROW;
    if(ctx->rx_video_rtp_ring_size > 0)
        ops.type = ST20_TYPE_RTP_LEVEL;
    else
        ops.type = video ? video->info.type : ST20_TYPE_FRAME_LEVEL;

    ops.flags = ST20_RX_FLAG_DMA_OFFLOAD;
    if(video && video->info.video_format == VIDEO_FORMAT_AUTO) 
    {
        ops.flags |= ST20_RX_FLAG_AUTO_DETECT;
        ops.width = 1920;
        ops.height = 1080;
        ops.fps = ST_FPS_P59_94;
    }
    else
    {
        ops.width = video ? st_app_get_width(video->info.video_format) : 1920;
        ops.height = video ? st_app_get_height(video->info.video_format) : 1080;
        ops.fps = video ? st_app_get_fps(video->info.video_format) : ST_FPS_P59_94;
    }
    ops.fmt = video ? video->info.pg_format : ST20_FMT_YUV_422_10BIT;
    ops.payload_type = video ? video->base.payload_type : ST_APP_PAYLOAD_TYPE_VIDEO;
    ops.interlaced = video ? st_app_get_interlaced(video->info.video_format) : false;
    ops.notify_frame_ready = app_rx_video_frame_ready;
    ops.slice_lines = ops.height / 32;
    //ops.notify_slice_ready = app_rx_video_slice_ready;
    //ops.notify_rtp_ready = app_rx_video_rtp_ready;
    ops.notify_detected = app_rx_video_detected;
    ops.framebuff_cnt = s->framebuff_cnt;
    ops.rtp_ring_size = ctx->rx_video_rtp_ring_size;
    if(!ops.rtp_ring_size) ops.rtp_ring_size = 1024;
    if(ops.type == ST20_TYPE_SLICE_LEVEL)
    {
        ops.flags |= ST20_RX_FLAG_RECEIVE_INCOMPLETE_FRAME;
        s->slice = true;
    }
    else
    {
        s->slice = false;
    }
    if(ctx->enable_hdr_split) ops.flags |= ST20_RX_FLAG_HDR_SPLIT;

    st_pthread_mutex_init(&s->st20_wake_mutex, NULL);
    st_pthread_cond_init(&s->st20_wake_cond, NULL);

    if(st_pmd_by_port_name(ops.port[ST_PORT_P]) == ST_PMD_DPDK_AF_XDP)
    {
        snprintf(s->st20_dst_url, ST_APP_URL_MAX_LEN, "st_app%d_%d_%d_%s.yuv", idx, ops.width,
                ops.height, ops.port[ST_PORT_P]);
    }
    else
    {
        uint32_t soc = 0, b = 0, d = 0, f = 0;
        sscanf(ops.port[ST_PORT_P], "%x:%x:%x.%x", &soc, &b, &d, &f);
        snprintf(s->st20_dst_url, ST_APP_URL_MAX_LEN,
                    "st_app%d_%d_%d_%02x_%02x_%02x-%02x.yuv", idx, ops.width, ops.height, soc, b,
                    d, f);
    }
    ret = st20_get_pgroup(ops.fmt, &s->st20_pg);
    if(ret < 0) return ret;
    s->user_pg.fmt = video ? video->user_pg_format : USER_FMT_MAX;
    if(s->user_pg.fmt != USER_FMT_MAX)
    {
        ret = user_get_pgroup(s->user_pg.fmt, &s->user_pg);
        if (ret < 0) return ret;
        ops.uframe_size = ops.width * ops.height * s->user_pg.size / s->user_pg.coverage;
        //ops.uframe_pg_callback = pg_convert_callback;
    }

    s->width = ops.width;
    s->height = ops.height;
    if(ops.interlaced) s->height >>= 1;
    s->expect_fps = st_frame_rate(ops.fps);
    s->pcapng_max_pkts = ctx->pcapng_max_pkts;

    s->framebuff_producer_idx = 0;
    s->framebuff_consumer_idx = 0;
    s->framebuffs = (struct st_rx_frame*)st_app_zmalloc(sizeof(*s->framebuffs) * s->framebuff_cnt);
    if(!s->framebuffs) return -ENOMEM;
    for(uint16_t j = 0; j < s->framebuff_cnt; j++)
    {
        s->framebuffs[j].frame = NULL;
    }

    if(ctx->has_sdl && video && video->display)
    {
        struct st_display* d = (struct st_display*)st_app_zmalloc(sizeof(struct st_display));
        //ret = st_app_init_display(d, s->idx, s->width, s->height, ctx->ttf_file);
        if(ret < 0)
        {
            logger->error("{}({}), st_app_init_display fail {}", __func__, idx, ret);
            app_rx_video_uinit(s);
            return -EIO;
        }
        s->display = d;
    }

    s->measure_latency = video ? video->measure_latency : true;

    handle = st20_rx_create(ctx->st, &ops);
    if(!handle)
    {
        logger->error("{}({}), st20_rx_create fail", __func__, idx);
        app_rx_video_uinit(s);
        return -EIO;
    }
    s->handle = handle;

    s->st20_frame_size = st20_rx_get_framebuffer_size(handle);
    // rx output handle
    s->rx_output = ctx->rx_output[video->rx_output_id];
    s->output_info = ctx->output_info[video->rx_output_id];


    ret = app_rx_video_init_frame_thread(s);
    if(ret < 0)
    {
        logger->error("{}({}), app_rx_video_init_thread fail {}, type {}", __func__, idx, ret, ops.type);
        app_rx_video_uinit(s);
        return -EIO;
    }

    s->stat_frame_received = 0;
    s->stat_last_time = st_app_get_monotonic_time();

    return 0;
}

int st_app_rx_video_sessions_init(struct st_app_context* ctx) 
{
    int ret, i;
    struct st_app_rx_video_session* s;
    int fb_cnt = ctx->rx_video_fb_cnt;
    if(fb_cnt <= 0) fb_cnt = 6;

    ctx->rx_video_sessions = (struct st_app_rx_video_session*)st_app_zmalloc(
        sizeof(struct st_app_rx_video_session) * ctx->rx_video_session_cnt);
    if(!ctx->rx_video_sessions) return -ENOMEM;
    for(i = 0; i < ctx->rx_video_session_cnt; i++) 
    {
        s = &ctx->rx_video_sessions[i];
        s->idx = i;
        s->st = ctx->st;
        s->framebuff_cnt = fb_cnt;
        s->st20_dst_fb_cnt = ctx->rx_video_file_frames;
        s->st20_dst_fd = -1;

        ret = app_rx_video_init(ctx, ctx->json_ctx ? &ctx->json_ctx->rx_video_sessions[i] : NULL, s);
        if(ret < 0)
        {
            logger->debug("{}({}), app_rx_video_init fail {}", __func__, i, ret);
            return ret;
        }
    }
}

int st_app_rx_video_sessions_uinit(struct st_app_context* ctx)
{
    int i;
    struct st_app_rx_video_session* s;
    if(!ctx->rx_video_sessions) return 0;
    for (i = 0; i < ctx->rx_video_session_cnt; i++)
    {
        s = &ctx->rx_video_sessions[i];
        app_rx_video_uinit(s);
    }
    st_app_free(ctx->rx_video_sessions);

    return 0;
}

static int app_rx_video_stat(struct st_app_rx_video_session* s)
{
    uint64_t cur_time_ns = st_app_get_monotonic_time();
    #ifdef DEBUG
        double time_sec = (double)(cur_time_ns - s->stat_last_time) / NS_PER_S;
        double framerate = s->stat_frame_received / time_sec;
        logger->debug("{}({}), fps {}, {} frame received", __func__, s->idx, framerate,
            s->stat_frame_received);
    #endif
    if(s->measure_latency && s->stat_frame_received)
    {
        double latency_ms = (double)s->stat_latency_us_sum / s->stat_frame_received / 1000;
        logger->info("{}({}), avrage latency {}ms", __func__, s->idx, latency_ms);
        s->stat_latency_us_sum = 0;
    }
    s->stat_frame_received = 0;
    s->stat_last_time = cur_time_ns;

    return 0;
}

static int app_rx_video_result(struct st_app_rx_video_session* s)
{
    int idx = s->idx;
    uint64_t cur_time_ns = st_app_get_monotonic_time();
    double time_sec = (double)(cur_time_ns - s->stat_frame_frist_rx_time) / NS_PER_S;
    double framerate = s->stat_frame_total_received / time_sec;

    if(!s->stat_frame_total_received) return -EINVAL;

    logger->critical("{}({}), {}, fps {}, {} frame received", __func__, idx,
            ST_APP_EXPECT_NEAR(framerate, s->expect_fps, s->expect_fps * 0.05) ? "OK"
                                                                                : "FAILED",
            framerate, s->stat_frame_total_received);
    return 0;
}