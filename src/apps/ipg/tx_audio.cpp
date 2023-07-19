
#include <memory>

#include "core/util/logger.h"
#include "core/stream/input.h"
#include "core/frame/frame.h"
#include "tx_audio.h"

#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace seeder::core;

static int app_tx_audio_next_frame(void* priv, uint16_t* next_frame_idx,
                                   struct st30_tx_frame_meta* meta) 
{
    struct st_app_tx_audio_session* s = (st_app_tx_audio_session*)priv;
    int ret;
    uint16_t consumer_idx = s->framebuff_consumer_idx;
    struct st_tx_frame* framebuff = &s->framebuffs[consumer_idx];

    st_pthread_mutex_lock(&s->st30_wake_mutex);
    if(ST_TX_FRAME_READY == framebuff->stat) 
    {
        logger->debug("{}({}), next frame idx {}", __func__, s->idx, consumer_idx);
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
        /* not ready */
        ret = -EIO;
        logger->debug("{}({}), idx {} err stat {}", __func__, s->idx, consumer_idx, framebuff->stat);
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
        logger->debug("{}({}), done_idx {}", __func__, s->idx, frame_idx);
    }
    else
    {
        ret = -EIO;
        logger->error("{}({}), err status {} for frame {}", __func__, s->idx, framebuff->stat,
            frame_idx);
    }
    st_pthread_cond_signal(&s->st30_wake_cond);
    st_pthread_mutex_unlock(&s->st30_wake_mutex);

    s->st30_frame_done_cnt++;
    logger->debug("{}({}), framebuffer index {}", __func__, s->idx, frame_idx);

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

static int app_tx_audio_open_source(struct st_app_tx_audio_session* s) {
  if (!s->st30_pcap_input) {
    struct stat i;

    s->st30_source_fd = st_open(s->st30_source_url, O_RDONLY);
    if (s->st30_source_fd >= 0) {
      fstat(s->st30_source_fd, &i);

      uint8_t* m = (uint8_t*)mmap(NULL, i.st_size, PROT_READ, MAP_SHARED, s->st30_source_fd, 0);

      if (MAP_FAILED != m) {
        s->st30_source_begin = m;
        s->st30_frame_cursor = m;
        s->st30_source_end = m + i.st_size;
      } else {
        
        return -EIO;
      }
    } else {
 
      return -EIO;
    }
  }

  return 0;
}

static int app_tx_audio_init(struct st_app_context* ctx, st_json_audio_session_t* audio,
                             struct st_app_tx_audio_session* s)
{
    int idx = s->idx, ret;
    struct st30_tx_ops ops;
    char name[32];
    st30_tx_handle handle;
    memset(&ops, 0, sizeof(ops));

    s->framebuff_cnt = 2;
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

    s->st30_source_fd = -1;
    st_pthread_mutex_init(&s->st30_wake_mutex, NULL);
    st_pthread_cond_init(&s->st30_wake_cond, NULL);

    snprintf(name, 32, "app_tx_audio_%d", idx);
    ops.name = name;
    ops.priv = s;
    ops.num_port = audio ? audio->base.num_inf : ctx->para.num_ports;
    memcpy(ops.dip_addr[MTL_PORT_P],
            audio ? audio->base.ip[MTL_PORT_P] : ctx->tx_dip_addr[MTL_PORT_P], MTL_IP_ADDR_LEN);
    strncpy(ops.port[MTL_PORT_P],
            audio ? audio->base.inf[MTL_PORT_P].name : ctx->para.port[MTL_PORT_P],
            MTL_PORT_MAX_LEN);
    ops.udp_port[MTL_PORT_P] = audio ? audio->base.udp_port : (10100 + s->idx);
    if(ctx->has_tx_dst_mac[MTL_PORT_P])
    {
        memcpy(&ops.tx_dst_mac[MTL_PORT_P][0], ctx->tx_dst_mac[MTL_PORT_P], 6);
        ops.flags |= ST30_TX_FLAG_USER_P_MAC;
    }
    if(ops.num_port > 1)
    {
        memcpy(ops.dip_addr[MTL_PORT_R],
            audio ? audio->base.ip[MTL_PORT_R] : ctx->tx_dip_addr[MTL_PORT_R],
            MTL_IP_ADDR_LEN);
        strncpy(ops.port[MTL_PORT_R],
                audio ? audio->base.inf[MTL_PORT_R].name : ctx->para.port[MTL_PORT_R],
                MTL_PORT_MAX_LEN);
        ops.udp_port[MTL_PORT_R] = audio ? audio->base.udp_port : (10100 + s->idx);
        if(ctx->has_tx_dst_mac[MTL_PORT_R])
        {
            memcpy(&ops.tx_dst_mac[MTL_PORT_R][0], ctx->tx_dst_mac[MTL_PORT_R], 6);
            ops.flags |= ST30_TX_FLAG_USER_R_MAC;
        }
    }
    ops.get_next_frame = app_tx_audio_next_frame;
    ops.notify_frame_done = app_tx_audio_frame_done;
    //ops.notify_rtp_done = app_tx_audio_rtp_done;
    ops.framebuff_cnt = s->framebuff_cnt;
    ops.fmt = audio ? audio->info.audio_format : ST30_FMT_PCM16;
    ops.channel = audio ? audio->info.audio_channel : 2;
    ops.sampling = audio ? audio->info.audio_sampling : ST30_SAMPLING_48K;
    ops.ptime = audio ? audio->info.audio_ptime : ST30_PTIME_1MS;
    ops.sample_size = st30_get_sample_size(ops.fmt);
    ops.sample_num = st30_get_sample_num(ops.ptime, ops.sampling);
    s->pkt_len = ops.sample_size * ops.sample_num * ops.channel;
    if(ops.ptime == ST30_PTIME_4MS)
    {
        s->st30_frame_size =
            ops.sample_size * st30_get_sample_num(ST30_PTIME_4MS, ops.sampling) * ops.channel;
    }
    else
    {
        /* when ptime <= 1ms, set frame time to 1ms */
        s->st30_frame_size =
            ops.sample_size * st30_get_sample_num(ST30_PTIME_1MS, ops.sampling) * ops.channel;
    }
    ops.framebuff_size = s->st30_frame_size;
    ops.payload_type = audio ? audio->base.payload_type : ST_APP_PAYLOAD_TYPE_AUDIO;

    s->st30_pcap_input = false;
    //tx audio source
    s->tx_source = ctx->tx_sources[audio->tx_source_id];
    s->tx_source_id = audio->tx_source_id;
    s->source_info = std::make_shared<st_app_tx_source>(ctx->source_info[audio->tx_source_id]);

    // s->st30_pcap_input = false;
    ops.type = audio ? audio->info.type : ST30_TYPE_FRAME_LEVEL;
    handle = st30_tx_create(ctx->st, &ops);
    if (!handle) {
        logger->error("{}({}), st30_tx_create fail", __func__, idx);
        st_app_tx_audio_session_uinit(s);
        return -EIO;
    }

    s->st30_pcap_input = false;

    s->handle = handle;
    strncpy(s->st30_source_url, audio ? audio->info.audio_url : ctx->tx_audio_url,
            sizeof(s->st30_source_url));

    // ret = app_tx_audio_open_source(s);
    // if (ret < 0) {
    //     logger->error("%s(%d), app_tx_audio_session_open_source fail\n", __func__, idx);
    //     app_tx_audio_uinit(s);
    //     return ret;
    // }
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
    ctx->tx_audio_sessions.resize(ctx->json_ctx->tx_audio_sessions.size());
    for(std::size_t i = 0; i < ctx->tx_audio_sessions.size(); i++) 
    {
        auto &s = ctx->tx_audio_sessions[i];
        s = std::shared_ptr<st_app_tx_audio_session>(new st_app_tx_audio_session {});
        ret = st_app_tx_audio_sessions_init(ctx, &ctx->json_ctx->tx_audio_sessions[i], s.get());
        if (ret) return ret;
    }

    return 0;
}

int st_app_tx_audio_sessions_add(struct st_app_context* ctx, st_json_context_t *new_json_ctx) {
    int ret = 0;
    for (auto &json_audio : new_json_ctx->tx_audio_sessions) {
        auto s = std::shared_ptr<st_app_tx_audio_session>(new st_app_tx_audio_session {});
        ret = st_app_tx_audio_sessions_init(ctx, &json_audio, s.get());
        if (ret) return ret;
        ctx->tx_audio_sessions.push_back(s);
    }
    return ret;
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
