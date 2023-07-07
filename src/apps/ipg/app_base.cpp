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
        for(auto s : ctx->rx_output)
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