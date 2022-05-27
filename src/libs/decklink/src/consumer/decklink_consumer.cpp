/**
 * @file decklink_consumer.h
 * @author 
 * @brief decklink consumer, send SDI frame by decklink card
 * @version 
 * @date 2022-05-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#include "consumer/decklink_consumer.h"
#include "util/logger.h"
#include "util.h"
#include "util/color_conversion.h"

using namespace seeder::util;
using namespace seeder::core;
using namespace seeder::decklink::util;
namespace seeder::decklink {
    /**
     * @brief Construct a new decklink consumer::decklink consumer object.
     * initialize deckllink device and start output stream
     */
    decklink_consumer::decklink_consumer(int device_id, video_format_desc& format_desc)
    :decklink_index_(device_id)
    ,format_desc_(format_desc)
    {
        HRESULT result;
        bmd_mode_ = get_decklink_video_format(format_desc_.format);
        video_flags_ = 0;
        
        // get the decklink device
        decklink_ = get_decklink(decklink_index_);
        if(decklink_ == nullptr)
        {
            logger->error("Failed to get DeckLink device. Make sure that the device ID is valid");
            throw std::runtime_error("Failed to get DeckLink device.");
        }

        // get output interface of the decklink device
        result = decklink_->QueryInterface(IID_IDeckLinkOutput, (void**)&output_);
        if(result != S_OK)
        {
            logger->error("Failed to get DeckLink output interface.");
            throw std::runtime_error("Failed to get DeckLink output interface.");
        }

        // get decklink output mode
        result = output_->GetDisplayMode(bmd_mode_, &display_mode_);
        if(result != S_OK)
        {
           logger->error("Failed to get DeckLink output mode.");
           throw std::runtime_error("Failed to get DeckLink output mode.");
        }

        // enable video output
        result = output_->EnableVideoOutput(bmd_mode_, bmdVideoOutputFlagDefault);
        if (result != S_OK)
        {
            logger->error("Unable to enable video output");
            throw std::runtime_error("Unable to enable video output");
        }

        // create display frame
        auto outputBytesPerRow = display_mode_->GetWidth() * 4;//bmdFormat8BitBGRA //((display_mode_->GetWidth() + 47) / 48) * 128; // bmdFormat10BitYUV
        result = output_->CreateVideoFrame((int32_t)display_mode_->GetWidth(),
													  (int32_t)display_mode_->GetHeight(),
													  outputBytesPerRow,
													  pixel_format_,
													  bmdFrameFlagDefault,
													  &playbackFrame_);
        if(result != S_OK)
        {
           logger->error("Unable to create video frame.");
           throw std::runtime_error("Unable to create video frame.");
        }

        // // ffmpeg sws context, AV_PIX_FMT_UYVY422 to AV_PIX_FMT_RGBA
        // sws_ctx_ = sws_getContext(format_desc.width, format_desc.height, AV_PIX_FMT_UYVY422,
        //                     format_desc.width, format_desc.height, AV_PIX_FMT_BGRA, 
        //                     SWS_BICUBIC, NULL, NULL, NULL);
        
        // dst_frame_ = av_frame_alloc();
        // int buf_size = av_image_get_buffer_size(AV_PIX_FMT_BGRA, format_desc.width, format_desc.height, 1);
        // dst_frame_buffer_ = (uint8_t*)av_malloc(buf_size);
        // av_image_fill_arrays(dst_frame_->data, dst_frame_->linesize, dst_frame_buffer_, AV_PIX_FMT_BGRA, 
        //                     format_desc.width, format_desc.height, 1);
    }
    
    decklink_consumer::~decklink_consumer()
    {
        if(display_mode_ != nullptr) display_mode_->Release();

        if(output_ != nullptr) output_->Release();

        if(decklink_ != nullptr) decklink_->Release();

        if(playbackFrame_ != nullptr) playbackFrame_->Release();

        if(sws_ctx_) sws_freeContext(sws_ctx_);

        if(dst_frame_)  av_frame_free(&dst_frame_);

        if(dst_frame_buffer_) av_free(dst_frame_buffer_);
    }

    /**
     * @brief start decklink output
     * 
     */
    void decklink_consumer::start()
    {
        HRESULT result;
        abort_ = false;

        while(!abort_)
        {
            std::shared_ptr<buffer> frame;
            {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                if(frame_buffer_.size() < 1)
                {
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
                    continue;
                }
                frame = frame_buffer_[0];
                frame_buffer_.pop_front();
            }

            // convert frame format
            // sws_scale(sws_ctx_, (const uint8_t *const *)frame->data,
            //                         frame->linesize, 0, format_desc_.height, dst_frame_->data, dst_frame_->linesize);
            // convert uyvy422 10bit to decklink bgra 8 bit
            auto bgra_frame = ycbcr422_to_bgra(frame, format_desc_);

            // fill playback frame
            uint8_t* deckLinkBuffer = nullptr;
            if (playbackFrame_->GetBytes((void**)&deckLinkBuffer) != S_OK)
            {
                logger->error("Could not get DeckLinkVideoFrame buffer pointer");
            }
            //deckLinkBuffer = reinterpret_cast<uint8_t*>(dst_frame_->data[0]);
            memset(deckLinkBuffer, 0, playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());
            memcpy(deckLinkBuffer, bgra_frame->begin(), playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());

            result = output_->DisplayVideoFrameSync(playbackFrame_);
			if (result != S_OK)
			{
				logger->error("Unable to display video output");
			}
        }
    }

    /**
     * @brief stop decklink capture
     * 
     */
    void decklink_consumer::stop()
    {
        abort_ = true;
    }

    /**
     * @brief send the frame object to the frame_buffer
     * if the frame buffer is full, dicard the last frame
     * 
     */
    void decklink_consumer::send_frame(std::shared_ptr<buffer> frame)
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the last frame
        }
        frame_buffer_.push_back(frame);
    }
}