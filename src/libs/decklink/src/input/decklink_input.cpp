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

#include "input/decklink_input.h"
#include "core/util/logger.h"
#include "util.h"

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
    ,pixel_format_(bmdFormat8BitYUV)
    ,video_flags_(bmdVideoInputFlagDefault)
    {
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
            if(result == E_FAIL)
            {
                logger->error("E_FAIL");
            }
            else if(result == E_INVALIDARG)
            {
                logger->error("E_INVALIDARG");
            }
            else if(result == E_ACCESSDENIED)
            {
                logger->error("E_ACCESSDENIED");
            }
            else if(result == E_OUTOFMEMORY)
            {
                logger->error("E_OUTOFMEMORY");
            }
            else 
            {
                logger->error("unknow");
            }
            

            logger->error("Failed to enable DeckLink video input.");
            throw std::runtime_error("Failed to enable DeckLink video input.");
        }
        // TODO parameters from config file
        result = input_->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 8);
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
        input_->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 8);
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
            frm->video = vframe;
        }

        if(audio)
        {
            auto aframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
            aframe->format = AV_SAMPLE_FMT_S16;
            aframe->channels = format_desc_.audio_channels;
            aframe->sample_rate = format_desc_.audio_sample_rate;

            void* audio_bytes = nullptr;
            if(audio->GetBytes(&audio_bytes) == S_OK && audio_bytes)
            {
                audio->AddRef();
                aframe = std::shared_ptr<AVFrame>(aframe.get(), [aframe, audio](AVFrame* ptr) { audio->Release(); });
                aframe->data[0] = reinterpret_cast<uint8_t*>(audio_bytes);
                aframe->nb_samples  = audio->GetSampleFrameCount();
                aframe->linesize[0] = aframe->nb_samples * aframe->channels *
                                       av_get_bytes_per_sample(static_cast<AVSampleFormat>(aframe->format));

                BMDTimeValue duration;
                if (audio->GetPacketTime(&in_audio_pts, AV_TIME_BASE)) 
                {
                    aframe->pts = in_audio_pts; //need bugging to ditermine the in_audio_pts meets the requirement
                }
            }
            frm->video = aframe;
        }

        this->set_frame(frm);

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
            logger->error("The frame is discarded");
        }
        frame_buffer_.push_back(frm);
    }

    /**
     * @brief Get a frame from this input stream
     * 
     */
    std::shared_ptr<frame> decklink_input::get_frame()
    {
        std::shared_ptr<frame> frm;
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            if(frame_buffer_.size() < 1)
                return last_frame_;

            frm = frame_buffer_[0];
            frame_buffer_.pop_front();
            last_frame_ = frm;
        }

        return frm;
    }

}