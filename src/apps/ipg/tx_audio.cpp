
#include <memory>

#include "core/util/logger.h"
#include "core/stream/input.h"
#include "core/frame/frame.h"
#include "tx_audio.h"

#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace seeder::core;

void st_app_tx_audio_session_reset_stat(struct st_app_tx_audio_session* s) {
    s->next_frame_stat = 0;
    s->next_frame_not_ready_stat = 0;
    s->frame_done_stat = 0;
    s->build_frame_stat = 0;
}

static int app_tx_audio_next_frame(void* priv, uint16_t* next_frame_idx,
                                   struct st30_tx_frame_meta* meta) 
{
    struct st_app_tx_audio_session* s = (st_app_tx_audio_session*)priv;
    int ret;
    uint16_t consumer_idx = s->framebuff_consumer_idx;
    struct st_tx_frame* framebuff = &s->framebuffs[consumer_idx];

    st_pthread_mutex_lock(&s->st30_wake_mutex);
    s->next_frame_stat++;
    if(ST_TX_FRAME_READY == framebuff->stat) 
    {
        logger->trace("{}({}), next frame idx {}", __func__, s->idx, consumer_idx);
        ret = 0;
        framebuff->stat = ST_TX_FRAME_IN_TRANSMITTING;
        *next_frame_idx = consumer_idx;
        /* point to next */
        consumer_idx++;
        if(consumer_idx >= s->framebuff_cnt) consumer_idx = 0;
        s->framebuff_consumer_idx = consumer_idx;
    }
    else 
    {
        s->next_frame_not_ready_stat++;
        /* not ready */
        ret = -EIO;
        logger->trace("{}({}), idx {} err stat {}", __func__, s->idx, consumer_idx, framebuff->stat);
    }
    st_pthread_cond_signal(&s->st30_wake_cond);
    st_pthread_mutex_unlock(&s->st30_wake_mutex);

    return ret;
}

static int app_tx_audio_frame_done(void* priv, uint16_t frame_idx,
                                   struct st30_tx_frame_meta* meta) 
{
    struct st_app_tx_audio_session* s = (st_app_tx_audio_session*)priv;
    int ret;
    struct st_tx_frame* framebuff = &s->framebuffs[frame_idx];

    st_pthread_mutex_lock(&s->st30_wake_mutex);
    if(ST_TX_FRAME_IN_TRANSMITTING == framebuff->stat) 
    {
        ret = 0;
        framebuff->stat = ST_TX_FRAME_FREE;
        logger->trace("{}({}), done_idx {}", __func__, s->idx, frame_idx);
    }
    else
    {
        ret = -EIO;
        logger->error("{}({}), err status {} for frame {}", __func__, s->idx, framebuff->stat,
            frame_idx);
    }
    st_pthread_cond_signal(&s->st30_wake_cond);
    st_pthread_mutex_unlock(&s->st30_wake_mutex);
    s->frame_done_stat++;
    s->st30_frame_done_cnt++;
    logger->trace("{}({}), framebuffer index {}", __func__, s->idx, frame_idx);

    return ret;
}

static void app_tx_audio_build_frame(struct st_app_tx_audio_session* s, void* frame,
                                     size_t frame_size) 
{
//     int ret = 0;
//     uint8_t* src = s->st30_frame_cursor;
//     // auto f = s->tx_source->get_audio_frame();
//     // if(!f) return;
//     uint8_t* dst = (uint8_t*)frame;
    
//     if (s->st30_frame_cursor + frame_size > s->st30_source_end) {
//     int len = s->st30_source_end - s->st30_frame_cursor;
//     len = len / s->pkt_len * s->pkt_len;
//     if (len) memcpy(dst, s->st30_frame_cursor, len);
//     /* wrap back in the end */
//     memcpy(dst + len, s->st30_source_begin, frame_size - len);
//     s->st30_frame_cursor = s->st30_source_begin + frame_size - len;
//   } else {
//     memcpy(dst, src, s->st30_frame_size);
//     s->st30_frame_cursor += s->st30_frame_size;
//   }

    auto f = s->tx_source->get_audio_frame_slice();
    
    uint8_t* dst = (uint8_t*)frame;
    uint8_t* src = (uint8_t*)f->begin();

    mtl_memcpy(dst, src, s->st30_frame_size);
      
   s->build_frame_stat++;
}

int st_app_tx_audio_session_uinit(struct st_app_tx_audio_session* s)
{
    int ret;

    s->st30_app_thread_stop = true;
    if(s->st30_app_thread)
    {
        /* wake up the thread */
        st_pthread_mutex_lock(&s->st30_wake_mutex);
        st_pthread_cond_signal(&s->st30_wake_cond);
        st_pthread_mutex_unlock(&s->st30_wake_mutex);
        logger->info("{}({}), wait app thread stop", __func__, s->idx);
        pthread_join(s->st30_app_thread, NULL);
    }

    //app_tx_audio_stop_source(s);
    if(s->handle)
    {
        ret = st30_tx_free(s->handle);
        if (ret < 0) 
            logger->error("{}}({}), st30_tx_free fail {}", __func__, s->idx, ret);
        s->handle = NULL;
    }

    //app_tx_audio_close_source(s);
    if(s->framebuffs)
    {
        st_app_free(s->framebuffs);
        s->framebuffs = NULL;
    }

    st_pthread_mutex_destroy(&s->st30_wake_mutex);
    st_pthread_cond_destroy(&s->st30_wake_cond);

    return 0;
}

static void* app_tx_audio_frame_thread(void* arg) 
{
    struct st_app_tx_audio_session* s = (st_app_tx_audio_session*)arg;
    int idx = s->idx;
    uint16_t producer_idx;
    struct st_tx_frame* framebuff;

    logger->debug("{}({}), start", __func__, idx);
    while(!s->st30_app_thread_stop)
    {
        st_pthread_mutex_lock(&s->st30_wake_mutex);
        producer_idx = s->framebuff_producer_idx;
        framebuff = &s->framebuffs[producer_idx];
        if(ST_TX_FRAME_FREE != framebuff->stat) 
        {
            /* not in free */
            if(!s->st30_app_thread_stop)
                st_pthread_cond_wait(&s->st30_wake_cond, &s->st30_wake_mutex);
            st_pthread_mutex_unlock(&s->st30_wake_mutex);
            continue;
        }
        st_pthread_mutex_unlock(&s->st30_wake_mutex);

        void* frame_addr = st30_tx_get_framebuffer(s->handle, producer_idx);
        app_tx_audio_build_frame(s, frame_addr, s->st30_frame_size);

        st_pthread_mutex_lock(&s->st30_wake_mutex);
        framebuff->size = s->st30_frame_size;
        framebuff->stat = ST_TX_FRAME_READY;
        /* point to next */
        producer_idx++;
        if (producer_idx >= s->framebuff_cnt) producer_idx = 0;
        s->framebuff_producer_idx = producer_idx;
        st_pthread_mutex_unlock(&s->st30_wake_mutex);
    }
    logger->info("{}({}), stop", __func__, idx);

    return NULL;
}

static int app_tx_audio_init(struct st_app_context* ctx, st_json_audio_session_t* audio,
                             struct st_app_tx_audio_session* s)
{
    int idx = s->idx, ret;
    struct st30_tx_ops ops;
    char name[32];
    st30_tx_handle handle;
    memset(&ops, 0, sizeof(ops));

    s->framebuff_cnt = 20;
    s->st30_seq_id = 1;

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

    //tx audio source
    s->tx_source = ctx->tx_sources[audio->base.id];
    s->tx_source_id = audio->base.id;
    s->source_info = std::make_shared<st_app_tx_source>(ctx->source_info[audio->base.id]);

    s->st30_source_fd = -1;
    st_pthread_mutex_init(&s->st30_wake_mutex, NULL);
    st_pthread_cond_init(&s->st30_wake_cond, NULL);

    snprintf(name, 32, "TX Audio SDI %d", s->source_info->device_id);
    ops.name = name;
    ops.priv = s;
    ops.num_port = audio->base.num_inf;
    memcpy(ops.dip_addr[MTL_PORT_P], audio->base.ip[MTL_PORT_P], MTL_IP_ADDR_LEN);
    strncpy(ops.port[MTL_PORT_P], audio->base.inf[MTL_PORT_P].name, MTL_PORT_MAX_LEN);
    ops.udp_port[MTL_PORT_P] = audio->base.udp_port[0];
    if(ops.num_port > 1)
    {
        memcpy(ops.dip_addr[MTL_PORT_R],
            audio->base.ip[MTL_PORT_R],
            MTL_IP_ADDR_LEN);
        strncpy(ops.port[MTL_PORT_R],
                audio->base.inf[MTL_PORT_R].name,
                MTL_PORT_MAX_LEN);
        ops.udp_port[MTL_PORT_R] = audio->base.udp_port[1];
    }
    ops.get_next_frame = app_tx_audio_next_frame;
    ops.notify_frame_done = app_tx_audio_frame_done;
    //ops.notify_rtp_done = app_tx_audio_rtp_done;
    ops.framebuff_cnt = s->framebuff_cnt;
    ops.fmt = audio->info.audio_format;
    ops.channel = audio->info.audio_channel;
    ops.sampling = audio->info.audio_sampling;
    ops.ptime = audio->info.audio_ptime;
    /* ops.sample_size = st30_get_sample_size(ops.fmt);
    ops.sample_num = st30_get_sample_num(ops.ptime, ops.sampling); */
    s->pkt_len = st30_get_packet_size(ops.fmt, ops.ptime, ops.sampling, ops.channel);
    int pkt_per_frame = 1;

    double pkt_time = st30_get_packet_time(ops.ptime);
    /* when ptime <= 1ms, set frame time to 1ms */
    constexpr int NS_PER_MS = (1000 * 1000);
    if (pkt_time < NS_PER_MS) {
        pkt_per_frame = NS_PER_MS / pkt_time;
    }

    s->st30_frame_size = pkt_per_frame * s->pkt_len;
    ops.framebuff_size = s->st30_frame_size;
    ops.payload_type = audio->base.payload_type;

    ops.type = ST30_TYPE_FRAME_LEVEL;
    handle = st30_tx_create(ctx->st, &ops);
    if (!handle) {
        logger->error("{}({}), st30_tx_create fail", __func__, idx);
        st_app_tx_audio_session_uinit(s);
        return -EIO;
    }

    s->handle = handle;

    //start frame thread to build frame
    ret = pthread_create(&s->st30_app_thread, NULL, app_tx_audio_frame_thread, s);
    if(ret < 0)
    {
        logger->error("{}, st30_app_thread create fail, err = {}", __func__, ret);
        st_app_tx_audio_session_uinit(s);
        return -EIO;
    }

    return 0;
}

static int st_app_tx_audio_sessions_init(struct st_app_context* ctx, st_json_audio_session_t *json_audio, st_app_tx_audio_session *s) {
    int ret;
    s->idx = ctx->next_tx_audio_session_idx++;
    ret = app_tx_audio_init(ctx, json_audio, s);
    if (ret < 0) logger->error("{}({}), app_tx_audio_init fail {}", __func__, s->idx, ret);
    return ret;
}

int st_app_tx_audio_sessions_init(struct st_app_context* ctx) 
{
    int ret;
    for(auto &json_audio : ctx->json_ctx->tx_audio_sessions) 
    {
        if (!json_audio.base.enable) {
            continue;
        }
        auto s = std::shared_ptr<st_app_tx_audio_session>(new st_app_tx_audio_session {});
        ret = st_app_tx_audio_sessions_init(ctx, &json_audio, s.get());
        if (ret) {
            continue;
        }
        ctx->tx_audio_sessions.push_back(s);
    }

    return 0;
}

int st_app_tx_audio_sessions_add(struct st_app_context* ctx, st_json_context_t *new_json_ctx) {
    int ret = 0;
    for (auto &json_audio : new_json_ctx->tx_audio_sessions) {
        if (!json_audio.base.enable) {
            continue;
        }
        auto s = std::shared_ptr<st_app_tx_audio_session>(new st_app_tx_audio_session {});
        ret = st_app_tx_audio_sessions_init(ctx, &json_audio, s.get());
        if (ret) {
            continue;
        }
        ctx->tx_audio_sessions.push_back(s);
    }
    return 0;
}


int st_app_tx_audio_sessions_uinit(struct st_app_context* ctx) 
{
    for (auto &s : ctx->tx_audio_sessions) {
        if (s) {
            st_app_tx_audio_session_uinit(s.get());
        }
    }
    ctx->tx_audio_sessions.clear();

    return 0;
}
