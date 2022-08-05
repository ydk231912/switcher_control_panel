/**
 * @file decklink_output.cpp
 * @author
 * @brief decklink output, send SDI frame by decklink card
 * @version 1.0
 * @date 2022-07-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#include "core/util/logger.h"
#include "util.h"
#include "core/util/color_conversion.h"
#include "output/decklink_output.h"

using namespace seeder::core;
using namespace seeder::decklink::util;
namespace seeder::decklink
{
    decklink_output::decklink_output(int device_id, core::video_format_desc format_desc)
    :decklink_index_(device_id),
    format_desc_(format_desc)
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

    decklink_output::~decklink_output()
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
     * @brief start output stream handle
     * 
     */
    void decklink_output::start()
    {
        HRESULT result;
        abort_ = false;

        std::thread([&](){
            while(!abort_)
            {
                auto frm = get_frame();

                // convert frame format
                // sws_scale(sws_ctx_, (const uint8_t *const *)frm->data,
                //                         frm->linesize, 0, format_desc_.height, dst_frame_->data, dst_frame_->linesize);
                // convert uyvy422 10bit to decklink bgra 8 bit
                auto bgra_frame = ycbcr422_to_bgra(frm->video_data[0], format_desc_);

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
        });
    }
        
    /**
     * @brief stop output stream handle
     * 
     */
    void decklink_output::stop()
    {
        abort_ = true;
    }

    /**
     * @brief push a frame into this output stream buffer
     * 
     */
    void decklink_output::set_frame(std::shared_ptr<core::frame> frm)
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the last frame
            logger->error("The frame be discarded");
        }
        frame_buffer_.push_back(frm);
        frame_cv_.notify_all();
    }

    /**
     * @brief Get a frame from this output stream buffer
     * 
     */
    std::shared_ptr<core::frame> decklink_output::get_frame()
    {
        std::shared_ptr<core::frame> frm;
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_cv_.wait(lock, [this](){return !frame_buffer_.empty();});
        frm = frame_buffer_[0];
        frame_buffer_.pop_front();
        return frm;
    }
}