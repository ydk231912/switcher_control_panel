#include <mtl/mtl_api.h>
#include "app_base.h"
#include "tx_video.h"
#include "tx_audio.h"
#include "rx_video.h"
#include "rx_audio.h"

namespace {

int st_tx_video_source_stop(struct st_app_context* ctx)
{
    try
    {
        for(auto &[id, s] : ctx->tx_sources)
        {
            if(s) s->stop();        
        }
    }
    catch(const std::exception& e)
    {
        return -1;
    }

    return 0;
}

int st_rx_output_stop(struct st_app_context* ctx)
{
    try
    {
        for(auto &[id, s] : ctx->rx_output)
        {
            if(s) s->stop();        
        }
    }
    catch(const std::exception& e)
    {
        return -1;
    }

    return 0;
}

} // namespace

st_app_context::~st_app_context() {
    st_tx_video_source_stop(this);
    st_rx_output_stop(this);
    st_app_tx_video_sessions_uinit(this);
    st_app_tx_audio_sessions_uinit(this);
    st_app_rx_video_sessions_uinit(this);
    st_app_rx_audio_sessions_uinit(this);

    if(this->runtime_session)
        if(this->st) mtl_stop(this->st);
    
    if(this->st) 
    {
        for(int i = 0; i < ST_APP_MAX_LCORES; i++) 
        {
            if(this->lcore[i] >= 0) {
                mtl_put_lcore(this->st, this->lcore[i]);
                this->lcore[i] = -1;
            }
            if(this->rtp_lcore[i] >= 0) {
                mtl_put_lcore(this->st, this->rtp_lcore[i]);
                this->rtp_lcore[i] = -1;
            }
        }
        mtl_uninit(this->st);
        this->st = nullptr;
    }
}

void st_set_video_foramt(struct st_json_audio_info info, seeder::core::video_format_desc* desc)
{
    auto sample_size = st30_get_sample_size(info.audio_format); //ST30_FMT_PCM8 1 byte, ST30_FMT_PCM16 2 bytes, ST30_FMT_PCM24 3 bytes
    auto ptime = info.audio_ptime; // ST30_PTIME_1MS, ST30_PTIME_4MS
    desc->audio_channels = info.audio_channel;
    desc->sample_num = st30_get_sample_num(ptime, info.audio_sampling); //sample number per ms, ST30_PTIME_1MS + sampling 48k/s = 48
    if(ptime == ST30_PTIME_4MS)
    {
        desc->st30_frame_size = sample_size * st30_get_sample_num(ST30_PTIME_4MS, info.audio_sampling) * info.audio_channel;
        desc->st30_fps = 250;
    }
    else
    {
        desc->st30_frame_size = sample_size * st30_get_sample_num(ST30_PTIME_1MS, info.audio_sampling) * info.audio_channel;
        desc->st30_fps = 1000;
    }

    if(info.audio_sampling == ST30_SAMPLING_96K)
        desc->audio_sample_rate = 96000;
    else
        desc->audio_sample_rate = 48000;

    if(info.audio_format == ST30_FMT_PCM8)
        desc->audio_samples = 8;
    else if(info.audio_format == ST30_FMT_PCM24)
        desc->audio_samples = 24;
    else if(info.audio_format == ST31_FMT_AM824)
        desc->audio_samples = 32;
    else 
        desc->audio_samples = 16;

}