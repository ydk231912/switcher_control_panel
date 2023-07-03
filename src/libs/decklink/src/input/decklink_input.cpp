/**
 * @file decklink_input.cpp
 * @author 
 * @brief decklink stream input handle
 * @version 1.0
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

extern "C"
{
    #include <libavformat/avformat.h>
}

#include "decklink/input/decklink_input.h"
#include "core/util/logger.h"
#include "decklink/util.h"

using namespace seeder::decklink::util;
using namespace seeder::core;
namespace seeder::decklink
{
    /**
     * @brief Construct a new decklink input object.
     * initialize deckllink device and start input stream
     */
    decklink_input::decklink_input(int device_id, video_format_desc& format_desc)
    :decklink_index_(device_id - 1)
    ,format_desc_(format_desc)
    ,pixel_format_(bmdFormat10BitYUV) //bmdFormat8BitYUV
    ,video_flags_(bmdVideoInputFlagDefault)
    {
        sample_type_ = bmdAudioSampleType32bitInteger;
        if(format_desc_.audio_samples == 32)
            sample_type_ = bmdAudioSampleType32bitInteger;
        

        HRESULT result;
        bmd_mode_ = get_decklink_video_format(format_desc_.format);
        
        // get the decklink device
        decklink_ = get_decklink(decklink_index_);
        if(decklink_ == nullptr)
        {
            logger->error("Failed to get DeckLink device. Make sure that the device ID is valid");
            throw std::runtime_error("Failed to get DeckLink device.");
        }

        // get input interface of the decklink device
        result = decklink_->QueryInterface(IID_IDeckLinkInput, (void**)&input_);
        if(result != S_OK)
        {
            logger->error("Failed to get DeckLink input interface.");
            throw std::runtime_error("Failed to get DeckLink input interface.");
        }

        // get decklink input mode
        result =  input_->GetDisplayMode(bmd_mode_, &display_mode_);
        if(result != S_OK)
        {
           logger->error("Failed to get DeckLink input mode.");
           throw std::runtime_error("Failed to get DeckLink input mode.");
        }

        // set the capture callback
        result = input_->SetCallback(this);
        if(result != S_OK)
        {
            logger->error("Failed to set DeckLink input callback.");
            throw std::runtime_error("Failed to set DeckLink input callback.");
        }
    }

    decklink_input::~decklink_input()
    {
        stop();

        if(display_mode_ != nullptr)
            display_mode_->Release();

        if(input_ != nullptr)
            input_->Release();

        if(decklink_ != nullptr)
            decklink_->Release();
    }

    /**
     * @brief start input stream handle
     * 
     */
    void decklink_input::start()
    {
        // enable input and start streams
        HRESULT result = input_->EnableVideoInput(bmd_mode_, pixel_format_, video_flags_);
        if(result != S_OK)
        {
            ///// for debug 
            // if(result == E_FAIL)
            // {
            //     logger->error("E_FAIL");
            // }
            // else if(result == E_INVALIDARG)
            // {
            //     logger->error("E_INVALIDARG");
            // }
            // else if(result == E_ACCESSDENIED)
            // {
            //     logger->error("E_ACCESSDENIED");
            // }
            // else if(result == E_OUTOFMEMORY)
            // {
            //     logger->error("E_OUTOFMEMORY");
            // }
            // else 
            // {
            //     logger->error("unknow");
            // }

            logger->error("Failed to enable DeckLink video input.");
            throw std::runtime_error("Failed to enable DeckLink video input.");
        }
        // TODO parameters from config file
        result = input_->EnableAudioInput(bmdAudioSampleRate48kHz, sample_type_, format_desc_.audio_channels);
        if(result != S_OK)
        {
            logger->error("Failed to enable DeckLink audio input.");
            throw std::runtime_error("Failed to enable DeckLink audio input.");
        }

        result = input_->StartStreams();
        if(result != S_OK)
        {
            logger->error("Failed to start DeckLink input streams.");
            throw std::runtime_error("Failed to start DeckLink input streams.");
        }
    }

    /**
     * @brief stop input stream handle
     * 
     */
    void decklink_input::stop()
    {
        if(input_ != nullptr)
        {
            input_->StopStreams();
            input_->DisableAudioInput();
            input_->DisableVideoInput();
        }
    }

    ULONG decklink_input::AddRef(void)
    {
        return 1;
    }

    ULONG decklink_input::Release(void)
    {
        return 1;
    }

    /**
     * @brief change display mode and restart input streams
     * 
     * @param events 
     * @param mode 
     * @param format_flags 
     * @return HRESULT 
     */
    HRESULT decklink_input::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, 
                                                        IDeckLinkDisplayMode* mode, 
                                                        BMDDetectedVideoInputFormatFlags format_flags)
    {
        display_mode_ = mode;
        input_->StopStreams();
        input_->EnableVideoInput(mode->GetDisplayMode(), pixel_format_, video_flags_);
        input_->EnableAudioInput(bmdAudioSampleRate48kHz, sample_type_, format_desc_.audio_channels);
        input_->FlushStreams();
        input_->StartStreams();
        
        return S_OK;
    }

    /**
     * @brief handle arrived frame from decklink card. 
     * push frame into the buffer, if the buffer is full, discard the old frame
     * 
     * @param video_frame 
     * @param audio_frame 
     * @return HRESULT 
     */
    HRESULT decklink_input::VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, 
                                                       IDeckLinkAudioInputPacket* audio)
    {
        BMDTimeValue in_video_pts = 0LL;
        BMDTimeValue in_audio_pts = 0LL;
        auto frm = std::make_shared<core::frame>(); 

        if(video)
        {
            auto vframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
            vframe->format = AV_PIX_FMT_UYVY422;
            vframe->width = video->GetWidth();
            vframe->height = video->GetHeight();
            vframe->interlaced_frame = display_mode_->GetFieldDominance() != bmdProgressiveFrame;
            vframe->top_field_first  = display_mode_->GetFieldDominance() == bmdUpperFieldFirst ? 1 : 0;
            vframe->key_frame        = 1;

            void* video_bytes = nullptr;
            if(video->GetBytes(&video_bytes) == S_OK && video_bytes)
            {
                video->AddRef();
                vframe = std::shared_ptr<AVFrame>(vframe.get(), [vframe, video](AVFrame* ptr) { video->Release(); });

                vframe->data[0] = reinterpret_cast<uint8_t*>(video_bytes);
                vframe->linesize[0] = video->GetRowBytes();

                BMDTimeValue duration;
                if (video->GetStreamTime(&in_video_pts, &duration, AV_TIME_BASE)) 
                {
                    vframe->pts = in_video_pts; //need bugging to ditermine the in_video_pts meets the requirement
                }
            }
            this->set_video_frame(vframe);
            //frm->video = vframe;
            
        }

        if(audio)
        {
            // a frame is 20ms:p50 or 40ms:p25
            // auto aframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
            // aframe->format = AV_SAMPLE_FMT_S16;
            // if(format_desc_.audio_samples == 32)
            //     aframe->format = AV_SAMPLE_FMT_S32;
            // aframe->channels = format_desc_.audio_channels;
            // aframe->sample_rate = format_desc_.audio_sample_rate;

            //  void* audio_bytes = nullptr;
            // if(audio->GetBytes(&audio_bytes) == S_OK && audio_bytes)
            // {
            //     audio->AddRef();
            //     aframe = std::shared_ptr<AVFrame>(aframe.get(), [aframe, audio](AVFrame* ptr) { audio->Release(); });
            //     aframe->data[0] = reinterpret_cast<uint8_t*>(audio_bytes);
            //     aframe->nb_samples  = audio->GetSampleFrameCount();
            //     aframe->linesize[0] = aframe->nb_samples * aframe->channels *
            //                            av_get_bytes_per_sample(static_cast<AVSampleFormat>(aframe->format));

            //     BMDTimeValue duration;
            //     if (audio->GetPacketTime(&in_audio_pts, AV_TIME_BASE))
            //     {
            //         aframe->pts = in_audio_pts; //need bugging to ditermine the in_audio_pts meets the requirement
            //     }
            // }
            // this->set_audio_frame(aframe);

            //one 20ms/40ms audio frame slices into multiple 125us/1ms/4ms frames
            int nb = format_desc_.st30_fps / format_desc_.fps; // slice number
            auto size = format_desc_.st30_frame_size;
            void* audio_bytes = nullptr;
            
            if(audio->GetBytes(&audio_bytes) == S_OK && audio_bytes)
            {
                for(int i = 0; i < nb; i++)
                {
                    auto aframe = std::make_shared<buffer>(format_desc_.st30_frame_size);
                    memcpy(aframe->begin(), audio_bytes + i * size , size);
                    this->set_audio_frame_slice(aframe);
                }
            }
        }

        return S_OK;
    }

    /**
     * @brief push the frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void decklink_input::set_frame(std::shared_ptr<frame> frm)
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the oldest frame
            //logger->error("The frame is discarded");
        }
        frame_buffer_.push_back(frm);
        frame_cv_.notify_all();
    }

    /**
     * @brief push the video frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void decklink_input::set_video_frame(std::shared_ptr<AVFrame> vframe)
    {
        std::lock_guard<std::mutex> lock(vframe_mutex_);
        if(vframe_buffer_.size() >= vframe_capacity_)
        {
            auto f = vframe_buffer_[0];
            vframe_buffer_.pop_front(); // discard the oldest frame
            //logger->warn("{}, The video frame is discarded, Decklink {}", __func__, decklink_index_ + 1);
        }
        vframe_buffer_.push_back(vframe);
        vframe_cv_.notify_all();
    }

    /**
     * @brief push the audio frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void decklink_input::set_audio_frame(std::shared_ptr<AVFrame> aframe)
    {
        std::lock_guard<std::mutex> lock(aframe_mutex_);
        if(aframe_buffer_.size() >= aframe_capacity_)
        {
            auto f = aframe_buffer_[0];
            aframe_buffer_.pop_front(); // discard the oldest frame
            //logger->error("{}, The audio frame is discarded", __func__);
        }
        aframe_buffer_.push_back(aframe);
        aframe_cv_.notify_all();
    }

    // /**
    //  * @brief Get a frame from this input stream
    //  * 
    //  */
    std::shared_ptr<frame> decklink_input::get_frame()
    {
        std::shared_ptr<core::frame> frm;
        std::unique_lock<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() > 0)
        {
            frm = frame_buffer_[0];
            frame_buffer_.pop_front();
            return frm;
        }
        frame_cv_.wait(lock, [this](){return !(frame_buffer_.empty());}); // block until the buffer is not empty
        frm = frame_buffer_[0];
        frame_buffer_.pop_front();
        return frm;

        // std::shared_ptr<frame> frm;
        // {
        //     std::lock_guard<std::mutex> lock(frame_mutex_);
        //     if(frame_buffer_.size() < 1)
        //         return last_frame_;

        //     frm = frame_buffer_[0];
        //     frame_buffer_.pop_front();
        //     last_frame_ = frm;
        // }

        // return frm;
    }

    /**
     * @brief Get a video frame from this input stream
     * 
     */
    std::shared_ptr<AVFrame> decklink_input::get_video_frame()
    {
        std::shared_ptr<AVFrame> vframe;
        std::unique_lock<std::mutex> lock(vframe_mutex_);
        if(vframe_buffer_.size() > 0)
        {
            vframe = vframe_buffer_[0];
            vframe_buffer_.pop_front();
            return vframe;
        }
        vframe_cv_.wait(lock, [this](){return !(vframe_buffer_.empty());}); // block until the buffer is not empty
        vframe = vframe_buffer_[0];
        vframe_buffer_.pop_front();
        return vframe;
    }

    /**
     * @brief Get a audio frame from this input stream
     * 
     */
    std::shared_ptr<AVFrame> decklink_input::get_audio_frame()
    {
        std::shared_ptr<AVFrame> aframe;
        std::unique_lock<std::mutex> lock(aframe_mutex_);
        if(aframe_buffer_.size() > 0)
        {
            aframe = aframe_buffer_[0];
            aframe_buffer_.pop_front();
            return aframe;
        }
        aframe_cv_.wait(lock, [this](){return !(aframe_buffer_.empty());}); // block until the buffer is not empty
        aframe = aframe_buffer_[0];
        aframe_buffer_.pop_front();
        return aframe;
    }

     /**
     * @brief push the audio frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void decklink_input::set_audio_frame_slice(std::shared_ptr<buffer> asframe)
    {
        std::lock_guard<std::mutex> lock(asframe_mutex_);
        if(asframe_buffer_.size() >= asframe_capacity_)
        {
            auto f = asframe_buffer_[0];
            asframe_buffer_.pop_front(); // discard the oldest frame
            logger->error("{}, The audio frame is discarded", __func__);
        }
        asframe_buffer_.push_back(asframe);
        asframe_cv_.notify_all();
    }

     /**
     * @brief Get a audio frame slice from this input stream
     * 
     */
    std::shared_ptr<buffer> decklink_input::get_audio_frame_slice()
    {
        std::shared_ptr<buffer> asframe;
        std::unique_lock<std::mutex> lock(asframe_mutex_);
        if(asframe_buffer_.size() > 0)
        {
            asframe = asframe_buffer_[0];
            asframe_buffer_.pop_front();
            return asframe;
        }
        asframe_cv_.wait(lock, [this](){return !(asframe_buffer_.empty());}); // block until the buffer is not empty
        asframe = asframe_buffer_[0];
        asframe_buffer_.pop_front();
        return asframe;
    }

}