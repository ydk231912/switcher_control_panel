/**
 * @file decklink_producer.cpp
 * @author 
 * @brief decklink producer, receive SDI frame from decklink card
 * @version 
 * @date 2022-04-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>

#include "decklink/producer/decklink_producer.h"
#include "core/util/logger.h"
#include "decklink/util.h"

using namespace seeder::core;
using namespace seeder::decklink::util;
namespace seeder::decklink {
    /**
     * @brief Construct a new decklink producer::decklink producer object.
     * initialize deckllink device and start input stream
     */
    decklink_producer::decklink_producer(int device_id, video_format_desc& format_desc)
    :decklink_index_(device_id)
    ,format_desc_(format_desc)
    {
        HRESULT result;
        pixel_format_ = bmdFormat8BitYUV;
        bmd_mode_ = get_decklink_video_format(format_desc_.format);
        video_flags_ = 0;
        
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
    
    decklink_producer::~decklink_producer()
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
     * @brief start decklink capture
     * 
     */
    void decklink_producer::start()
    {

        // enable input and start streams
        HRESULT result = input_->EnableVideoInput(bmd_mode_, pixel_format_, video_flags_);
        if(result != S_OK)
        {
            logger->error("Failed to enable DeckLink video input.");
            throw std::runtime_error("Failed to enable DeckLink video input.");
        }

        // result = input_->EnableAudioInput();
        // if(result != S_OK)
        // {
        //  logger->error("Failed to enable DeckLink audio input.");
        //  throw std::runtime_error("Failed to enable DeckLink audio input.");
        // }

        result = input_->StartStreams();
        if(result != S_OK)
        {
            logger->error("Failed to start DeckLink input streams.");
            throw std::runtime_error("Failed to start DeckLink input streams.");
        }
    }

    /**
     * @brief stop decklink capture
     * 
     */
    void decklink_producer::stop()
    {
        if(input_ != nullptr)
        {
            input_->StopStreams();
            input_->DisableAudioInput();
            input_->DisableVideoInput();
        }
    }

    ULONG decklink_producer::AddRef(void)
    {
        return 1;
    }

    ULONG decklink_producer::Release(void)
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
    HRESULT decklink_producer::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, 
                                                        IDeckLinkDisplayMode* mode, 
                                                        BMDDetectedVideoInputFormatFlags format_flags)
    {
        display_mode_ = mode;
        input_->StopStreams();
        input_->EnableVideoInput(mode->GetDisplayMode(), pixel_format_, video_flags_);
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
    HRESULT decklink_producer::VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, 
                                                       IDeckLinkAudioInputPacket* audio)
    {
        BMDTimeValue in_video_pts = 0LL;
        auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });

        if(video)
        {
            frame->format = AV_PIX_FMT_UYVY422;
            frame->width = video->GetWidth();
            frame->height = video->GetHeight();
            frame->interlaced_frame = display_mode_->GetFieldDominance() != bmdProgressiveFrame;
            frame->top_field_first  = display_mode_->GetFieldDominance() == bmdUpperFieldFirst ? 1 : 0;
            frame->key_frame        = 1;

            void* video_bytes = nullptr;
            if(video->GetBytes(&video_bytes) && video_bytes)
            {
                video->AddRef();
                frame = std::shared_ptr<AVFrame>(frame.get(), [frame, video](AVFrame* ptr) { video->Release(); });

                frame->data[0]     = reinterpret_cast<uint8_t*>(video_bytes);
                frame->linesize[0] = video->GetRowBytes();

                BMDTimeValue duration;
                if (video->GetStreamTime(&in_video_pts, &duration, AV_TIME_BASE)) 
                {
                    frame->pts = in_video_pts; //need bugging to ditermine the in_video_pts meets the requirement
                }
            }
        }

        // push the frame into the buffer, if the buffer is full, discard the oldest frame
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            if(frame_buffer_.size() >= frame_capacity_)
            {
                auto f = frame_buffer_[0];
                frame_buffer_.pop_front(); // discard the oldest frame
            }
            frame_buffer_.push_back(frame);
        }

        return S_OK;
    }

    /**
     * @brief Get the frame object from the frame_buffer
     * 
     * @return std::shared_ptr<AVFrame> 
     */
    std::shared_ptr<AVFrame> decklink_producer::get_frame()
    {
        std::shared_ptr<AVFrame> frame;
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            if(frame_buffer_.size() < 1)
                return last_frame_;

            frame = frame_buffer_[0];
            frame_buffer_.pop_front();
            last_frame_ = frame;
        }

        return frame;
    }
}