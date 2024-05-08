
#include "rx_audio.h"
#include "core/util/logger.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace seeder::core;


void st_app_rx_audio_session_reset_stat(struct st_app_rx_audio_session* s) {
    s->frame_receive_stat = 0;
    s->frame_drop_stat = 0;
}

static int app_rx_audio_rtp_ready(void* priv) {
  struct st_app_rx_audio_session* s = (st_app_rx_audio_session*)priv;

  st_pthread_mutex_lock(&s->st30_wake_mutex);
  st_pthread_cond_signal(&s->st30_wake_cond);
  st_pthread_mutex_unlock(&s->st30_wake_mutex);

  return 0;
}

static int app_rx_audio_enqueue_frame(struct st_app_rx_audio_session* s, void* frame, size_t size)
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
static void app_rx_audio_consume_frame(struct st_app_rx_audio_session* s, void* frame, size_t frame_size)
{
    auto output = s->rx_output;
    if(output)
    {
        output->consume_st_audio_frame(frame, frame_size);
        output->display_audio_frame();
    }
}

static void* app_rx_audio_frame_thread(void* arg)
{
    struct st_app_rx_audio_session* s = (st_app_rx_audio_session*)arg;
    int idx = s->idx;
    int consumer_idx;
    struct st_rx_frame* framebuff;

    logger->info("{}({}), start", __func__, idx);
    while(!s->st30_app_thread_stop)
    {
        st_pthread_mutex_lock(&s->st30_wake_mutex);
        consumer_idx = s->framebuff_consumer_idx;
        framebuff = &s->framebuffs[consumer_idx];
        if(!framebuff->frame)
        {
            /* no ready frame */
            if(!s->st30_app_thread_stop)
                st_pthread_cond_wait(&s->st30_wake_cond, &s->st30_wake_mutex);
            st_pthread_mutex_unlock(&s->st30_wake_mutex);
            continue;
        }
        st_pthread_mutex_unlock(&s->st30_wake_mutex);

        logger->trace("{}({}), frame idx {}", __func__, idx, consumer_idx);
        app_rx_audio_consume_frame(s, framebuff->frame, framebuff->size);
        st30_rx_put_framebuff(s->handle, framebuff->frame);
        /* point to next */
        st_pthread_mutex_lock(&s->st30_wake_mutex);
        framebuff->frame = NULL;
        consumer_idx++;
        if (consumer_idx >= s->framebuff_cnt) consumer_idx = 0;
        s->framebuff_consumer_idx = consumer_idx;
        st_pthread_mutex_unlock(&s->st30_wake_mutex);
    }
    logger->info("{}({}), stop", __func__, idx);

    return NULL;
}

static int app_rx_audio_init_frame_thread(struct st_app_rx_audio_session* s)
{
    int ret, idx = s->idx;


    ret = pthread_create(&s->st30_app_thread, NULL, app_rx_audio_frame_thread, s);
    if(ret < 0)
    {
        logger->error("{}({}), st0_app_thread create fail {}", __func__, ret, idx);
        return -EIO;
    }

    return 0;
}

static int app_rx_audio_frame_ready(void* priv, void* frame, struct st30_rx_frame_meta* meta)
{
    struct st_app_rx_audio_session* s = (st_app_rx_audio_session*)priv;
    int ret;

    if(!s->handle) return -EIO;

    s->stat_frame_total_received++;
    s->frame_receive_stat++;
    if(!s->stat_frame_frist_rx_time)
        s->stat_frame_frist_rx_time = st_app_get_monotonic_time();

    st_pthread_mutex_lock(&s->st30_wake_mutex);
    ret = app_rx_audio_enqueue_frame(s, frame, s->st30_frame_size);
    if(ret < 0)
    {
        s->frame_drop_stat++;
        /* free the queue */
        st30_rx_put_framebuff(s->handle, frame);
        st_pthread_mutex_unlock(&s->st30_wake_mutex);
        return ret;
    }
    st_pthread_cond_signal(&s->st30_wake_cond);
    st_pthread_mutex_unlock(&s->st30_wake_mutex);

    return 0;
}

static int app_rx_audio_init(struct st_app_context* ctx, st_json_audio_session_t* audio,
                        struct st_app_rx_audio_session* s)
{
    int idx = s->idx, ret;
    struct st30_rx_ops ops;
    char name[32];
    st30_rx_handle handle;
    memset(&ops, 0, sizeof(ops));

    s->rx_output = ctx->rx_output[audio->base.id];
    s->rx_output_id = audio->base.id;
    s->output_info = ctx->output_info[audio->base.id];

    snprintf(name, 32, "RX Audio SDI %d", s->output_info.device_id);
    ops.name = name;
    ops.priv = s;
    ops.num_port = audio->base.num_inf;
    memcpy(ops.sip_addr[MTL_PORT_P], audio->base.ip[MTL_PORT_P], MTL_IP_ADDR_LEN);
    strncpy(ops.port[MTL_PORT_P],
            audio->base.inf[MTL_PORT_P].name,
            MTL_PORT_MAX_LEN);
    ops.udp_port[MTL_PORT_P] = audio->base.udp_port[0];
    if(ops.num_port > 1)
    {
        memcpy(ops.sip_addr[MTL_PORT_R],
                audio->base.ip[MTL_PORT_R],
                MTL_IP_ADDR_LEN);
        strncpy(ops.port[MTL_PORT_R],
                audio->base.inf[MTL_PORT_R].name,
                MTL_PORT_MAX_LEN);
        ops.udp_port[MTL_PORT_R] = audio->base.udp_port[1];
    }
    
     ops.notify_frame_ready = app_rx_audio_frame_ready;
     ops.notify_rtp_ready = app_rx_audio_rtp_ready;

    ops.type = ST30_TYPE_FRAME_LEVEL;
    ops.fmt = audio->info.audio_format;
    ops.payload_type = audio->base.payload_type;
    ops.channel = audio->info.audio_channel;
    ops.sampling = audio->info.audio_sampling;
    ops.ptime = audio->info.audio_ptime;
    ops.sample_size = st30_get_sample_size(ops.fmt);
    ops.sample_num = st30_get_sample_num(ops.ptime, ops.sampling);
    s->pkt_len = ops.sample_size * ops.sample_num * ops.channel;
    if(ops.ptime == ST30_PTIME_4MS)
    {
        s->st30_frame_size = ops.sample_size * st30_get_sample_num(ST30_PTIME_4MS, ops.sampling) * ops.channel;
        s->expect_fps = 250.0;
    }
    else
    {
        /* when ptime <= 1ms, set frame time to 1ms */
        s->st30_frame_size = ops.sample_size * st30_get_sample_num(ST30_PTIME_1MS, ops.sampling) * ops.channel;
        s->expect_fps = 1000.0;
    }
    
    ops.framebuff_size = s->st30_frame_size;
    ops.framebuff_cnt = s->framebuff_cnt;
    
    // audio sample number per frame
    s->sample_num = ops.sample_num;
    s->framebuff_producer_idx = 0;
    s->framebuff_consumer_idx = 0;
    s->framebuffs = (struct st_rx_frame*)st_app_zmalloc(sizeof(*s->framebuffs) * s->framebuff_cnt);
    if(!s->framebuffs) return -ENOMEM;
    for(uint16_t j = 0; j < s->framebuff_cnt; j++)
    {
        s->framebuffs[j].frame = NULL;
    }

    st_pthread_mutex_init(&s->st30_wake_mutex, NULL);
    st_pthread_cond_init(&s->st30_wake_cond, NULL);

    handle = st30_rx_create(ctx->st, &ops);
    if(!handle) 
    {
        logger->error("{}({}), st30_rx_create fail", __func__, idx);
        return -EIO;
    }
    s->handle = handle;
    
    ret = app_rx_audio_init_frame_thread(s);
    if(ret < 0)
    {
        logger->error("{}({}), app_rx_audio_init_thread fail {}, type {}", __func__, idx, ret, ops.type);
        //app_rx_audio_uinit(s);
        return -EIO;
    }

    return 0;
}


int st_app_rx_audio_session_uinit(struct st_app_rx_audio_session* s)
{
    int ret, idx = s->idx;

    s->st30_app_thread_stop = true;
    if(s->st30_app_thread)
    {
        /* wake up the thread */
        st_pthread_mutex_lock(&s->st30_wake_mutex);
        st_pthread_cond_signal(&s->st30_wake_cond);
        st_pthread_mutex_unlock(&s->st30_wake_mutex);
        logger->info("{}({}), wait app thread stop", __func__, idx);
        pthread_join(s->st30_app_thread, NULL);
    }

    if(s->handle)
    {
        ret = st30_rx_free(s->handle);
        if(ret < 0) logger->error("{}({}), st30_rx_free fail {}", __func__, idx, ret);
        s->handle = NULL;
    }

    if(s->framebuffs) 
    {
        st_app_free(s->framebuffs);
        s->framebuffs = NULL;
    }

    st_pthread_mutex_destroy(&s->st30_wake_mutex);
    st_pthread_cond_destroy(&s->st30_wake_cond);

    return 0;
}

static int st_app_rx_audio_session_init(struct st_app_context* ctx, st_json_audio_session_t *json_audio, st_app_rx_audio_session *s) {
    s->idx = ctx->next_rx_audio_session_idx++;
    s->framebuff_cnt = 8;
    s->st30_ref_fd = -1;
    s->st = ctx->st;
    s->lcore = -1;
    s->ctx = ctx;
    int ret = app_rx_audio_init(ctx, json_audio, s);
    if(ret < 0) 
    {
        logger->error("{}({}), app_rx_audio_init fail {}", __func__, s->idx, ret);
        return ret;
    }
    return ret;
}

int st_app_rx_audio_sessions_init(struct st_app_context* ctx)
{
    int ret;
    for(auto &json_audio : ctx->json_ctx->rx_audio_sessions)
    {
        if (!json_audio.base.enable) {
            continue;
        }
        auto s = std::shared_ptr<st_app_rx_audio_session>(new st_app_rx_audio_session {});
        ret = st_app_rx_audio_session_init(ctx, &json_audio, s.get());
        if (ret) {
            continue;
        }
        ctx->rx_audio_sessions.push_back(s);
    }

    return 0;
}

int st_app_rx_audio_sessions_add(struct st_app_context* ctx, st_json_context_t *new_json_ctx) {
    int ret = 0;
    for (auto &json_audio : new_json_ctx->rx_audio_sessions) {
        if (!json_audio.base.enable) {
            continue;
        }
        auto s = std::shared_ptr<st_app_rx_audio_session>(new st_app_rx_audio_session {});
        ret = st_app_rx_audio_session_init(ctx, &json_audio, s.get());
        if (ret) {
            continue;
        }
        ctx->rx_audio_sessions.push_back(s);
    }
    return 0;
}


int st_app_rx_audio_sessions_uinit(struct st_app_context* ctx)
{
    for (auto &s : ctx->rx_audio_sessions) {
        if (s) {
            st_app_rx_audio_session_uinit(s.get());
        }
    }
    ctx->rx_audio_sessions.clear();

    return 0;
}


static int app_rx_audio_result(struct st_app_rx_audio_session* s)
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

int st_app_rx_audio_sessions_result(struct st_app_context* ctx)
{
    int ret = 0;
    for (auto &s : ctx->rx_audio_sessions) {
        ret += app_rx_audio_result(s.get());
    }

    return ret;
}
