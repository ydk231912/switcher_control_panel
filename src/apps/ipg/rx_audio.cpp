
#include "rx_audio.h"
#include "core/util/logger.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace seeder::core;


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

        logger->debug("{}({}), frame idx {}", __func__, idx, consumer_idx);
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
    if(!s->stat_frame_frist_rx_time)
        s->stat_frame_frist_rx_time = st_app_get_monotonic_time();

    st_pthread_mutex_lock(&s->st30_wake_mutex);
    ret = app_rx_audio_enqueue_frame(s, frame, s->st30_frame_size);
    if(ret < 0)
    {
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

    snprintf(name, 32, "app_rx_audio%d", idx);
    ops.name = name;
    ops.priv = s;
    ops.num_port = audio ? audio->base.num_inf : ctx->para.num_ports;
    memcpy(ops.sip_addr[MTL_PORT_P],
            audio ? audio->base.ip[MTL_PORT_P] : ctx->rx_sip_addr[MTL_PORT_P], MTL_IP_ADDR_LEN);
    strncpy(ops.port[MTL_PORT_P],
            audio ? audio->base.inf[MTL_PORT_P].name : ctx->para.port[MTL_PORT_P],
            MTL_PORT_MAX_LEN);
    ops.udp_port[MTL_PORT_P] = audio ? audio->base.udp_port : (10100 + s->idx);
    if(ops.num_port > 1)
    {
        memcpy(ops.sip_addr[MTL_PORT_R],
                audio ? audio->base.ip[MTL_PORT_R] : ctx->rx_sip_addr[MTL_PORT_R],
                MTL_IP_ADDR_LEN);
        strncpy(ops.port[MTL_PORT_R],
                audio ? audio->base.inf[MTL_PORT_R].name : ctx->para.port[MTL_PORT_R],
                MTL_PORT_MAX_LEN);
        ops.udp_port[MTL_PORT_R] = audio ? audio->base.udp_port : (10100 + s->idx);
    }
    
     ops.notify_frame_ready = app_rx_audio_frame_ready;
     ops.notify_rtp_ready = app_rx_audio_rtp_ready;

    ops.type = audio ? audio->info.type : ST30_TYPE_FRAME_LEVEL;
    ops.fmt = audio ? audio->info.audio_format : ST30_FMT_PCM16;
    ops.payload_type = audio ? audio->base.payload_type : ST_APP_PAYLOAD_TYPE_AUDIO;
    ops.channel = audio ? audio->info.audio_channel : 2;
    ops.sampling = audio ? audio->info.audio_sampling : ST30_SAMPLING_48K;
    ops.ptime = audio ? audio->info.audio_ptime : ST30_PTIME_1MS;
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
    ops.rtp_ring_size = ctx->rx_audio_rtp_ring_size ? ctx->rx_audio_rtp_ring_size : 16;
    
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

    strncpy(s->st30_ref_url, audio ? audio->info.audio_url : "null", sizeof(s->st30_ref_url));

    handle = st30_rx_create(ctx->st, &ops);
    if(!handle) 
    {
        logger->error("{}({}), st30_rx_create fail", __func__, idx);
        return -EIO;
    }
    s->handle = handle;
    s->rx_output = ctx->rx_output[audio->rx_output_id];
    s->rx_output_id = audio->rx_output_id;
    s->output_info = ctx->output_info[audio->rx_output_id];
    
    ret = app_rx_audio_init_frame_thread(s);
    if(ret < 0)
    {
        logger->error("{}({}), app_rx_audio_init_thread fail {}, type {}", __func__, idx, ret, ops.type);
        //app_rx_audio_uinit(s);
        return -EIO;
    }

    return 0;
}





static int app_rx_audio_uinit(struct st_app_rx_audio_session* s)
{
    int ret, idx = s->idx;

    if(s->framebuffs) 
    {
        st_app_free(s->framebuffs);
        s->framebuffs = NULL;
    }

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

    st_pthread_mutex_destroy(&s->st30_wake_mutex);
    st_pthread_cond_destroy(&s->st30_wake_cond);

    if(s->handle)
    {
        ret = st30_rx_free(s->handle);
        if(ret < 0) logger->error("{}({}), st30_rx_free fail {}", __func__, idx, ret);
        s->handle = NULL;
    }

    return 0;
}

int st_app_rx_audio_sessions_init(struct st_app_context* ctx)
{
    int ret;
    ctx->rx_audio_sessions.resize(ctx->json_ctx->rx_audio_sessions.size());
    for(auto i = 0; i < ctx->rx_audio_sessions.size(); i++)
    {
        auto &s = ctx->rx_audio_sessions[i];
        s = std::shared_ptr<st_app_rx_audio_session>(new st_app_rx_audio_session {});
        s->idx = ctx->next_rx_audio_session_idx++;
        s->framebuff_cnt = 2;
        s->st30_ref_fd = -1;

        ret = app_rx_audio_init(ctx, ctx->json_ctx ? &ctx->json_ctx->rx_audio_sessions[i] : NULL, s.get());
        if(ret < 0) 
        {
            logger->error("{}({}), app_rx_audio_init fail {}", __func__, i, ret);
            return ret;
        }
    }

    return 0;
}

// int st_app_rx_audio_sessions_init_add(struct st_app_context* ctx,st_json_context_t* c)
// {
//     int ret, i,count;
//      struct st_app_rx_audio_session* s;
//     // ctx->rx_audio_sessions = (struct st_app_rx_audio_session*)st_app_zmalloc(
//     //     sizeof(struct st_app_rx_audio_session) * c->rx_audio_session_cnt);
//     count = ctx->rx_audio_session_cnt;
//     if(!ctx->rx_audio_sessions) return -ENOMEM;

//     for(i = 0; i < c->rx_audio_session_cnt; i++)
//     {
//         count = count +i;
//         ctx->rx_anc_session_cnt+=1;
//         s = &ctx->rx_audio_sessions[count];
//         s->idx = i;
//         s->framebuff_cnt = 2;
//         s->st30_ref_fd = -1;
//         ret = app_rx_audio_init(ctx,c ? &c->rx_audio_sessions[i] : NULL,s);
//         if(ret < 0) 
//         {
//             logger->error("{}({}), app_rx_audio_init fail {}", __func__, i, ret);
//             return ret;
//         }
//     }

//     return 0;
// }


// int st_app_rx_audio_sessions_init_update(struct st_app_context* ctx,st_json_context_t* c)
// {
//     int ret, i;
//     struct st_app_rx_audio_session* s;
//     ctx->rx_audio_sessions = (struct st_app_rx_audio_session*)st_app_zmalloc(
//         sizeof(struct st_app_rx_audio_session) * c->rx_audio_session_cnt);
//     if(!ctx->rx_audio_sessions) return -ENOMEM;

//     for(i = 0; i < c->rx_audio_session_cnt; i++)
//     {
//         s = &ctx->rx_audio_sessions[i];
//         s->idx = i;
//         s->framebuff_cnt = 2;
//         s->st30_ref_fd = -1;
//         ret = app_rx_audio_init(ctx, c ? &c->rx_audio_sessions[i] : NULL, s);
//         if(ret < 0) 
//         {
//             logger->error("{}({}), app_rx_audio_init fail {}", __func__, i, ret);
//             return ret;
//         }
//     }

//     return 0;
// }


int st_app_rx_audio_sessions_uinit(struct st_app_context* ctx)
{
    for (auto &s : ctx->rx_audio_sessions) {
        if (s) {
            app_rx_audio_uinit(s.get());
        }
    }
    ctx->rx_audio_sessions.clear();

    return 0;
}


// int st_app_rx_audio_sessions_uinit_update(struct st_app_context* ctx,int id,st_json_context_t *c)
// {

//     int i,ret;
//     struct st_app_rx_audio_session* s;
//     struct st_app_rx_audio_session* s_new;
//     if(!ctx->rx_audio_sessions) return 0;

//     for(i = 0; i < ctx->rx_audio_session_cnt; i++)
//     {
//       if (ctx->rx_audio_sessions[i].id == id)
//       {
//         s = &ctx->rx_audio_sessions[i];
//         app_rx_audio_uinit(s);
//         for(int j = 0 ;j<c->rx_audio_session_cnt;j++){
//             if(c->rx_audio_sessions->rx_output_id == id){
//                 s_new->idx = i;
//                 s_new->framebuff_cnt = 2;
//                 s_new->st30_ref_fd = -1;
//                 ret = app_rx_audio_init(ctx, c ? &c->rx_audio_sessions[j] : NULL, s_new);
//             }
//         }
//         ctx->rx_audio_sessions[i] = *s_new;
//       }
//     }
//     return 0;
// }

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
