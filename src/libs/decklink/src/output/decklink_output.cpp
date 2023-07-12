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
#include "decklink/util.h"
#include "core/util/color_conversion.h"
#include "decklink/output/decklink_output.h"

using namespace seeder::core;
using namespace seeder::decklink::util;
namespace seeder::decklink
{
    decklink_output::decklink_output(
        const std::string &output_id, 
        int device_id, 
        const core::video_format_desc &format_desc, 
        const std::string &pixel_format
    )
    :output(output_id), decklink_index_(device_id - 1),
    format_desc_(format_desc)
    {
        pixel_format_string_ = pixel_format;
        if (pixel_format == "bmdFormat10BitYUV") {
            pixel_format_ = bmdFormat10BitYUV;
        }
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
        auto outputBytesPerRow = display_mode_->GetWidth() * 4; //display_mode_->GetWidth() * 4;//bmdFormat8BitBGRA //((display_mode_->GetWidth() + 47) / 48) * 128; // bmdFormat10BitYUV
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
        auto PerRow = ((display_mode_->GetWidth() + 47) / 48) * 128; //display_mode_->GetWidth() * 4;//bmdFormat8BitBGRA //((display_mode_->GetWidth() + 47) / 48) * 128; // bmdFormat10BitYUV
        result = output_->CreateVideoFrame((int32_t)display_mode_->GetWidth(),
													  (int32_t)display_mode_->GetHeight(),
													  PerRow,
													  bmdFormat10BitYUV,
													  bmdFrameFlagDefault,
													  &yuv10Frame_);
        vframe_buffer = (uint8_t*)malloc(format_desc_.height * ((display_mode_->GetWidth() + 47) / 48) * 128);

        // frame buffer pointer
        if (playbackFrame_->GetBytes((void**)&vframe_buffer) != S_OK)
        {
            logger->error("Could not get DeckLinkVideoFrame buffer pointer");
        }

        // enable audio output
        result = output_->EnableAudioOutput(format_desc.audio_sample_rate, format_desc.audio_samples, 
                                            format_desc.audio_channels, bmdAudioOutputStreamTimestamped);
        if (result != S_OK)
        {
            logger->error("Unable to enable video output");
            throw std::runtime_error("Unable to enable video output");
        }

        frameConverter = CreateVideoConversionInstance();
        if (frameConverter == nullptr)
        {
            logger->error("Unable to get Video Conversion interface");
            throw std::runtime_error("Unable to get Video Conversion interface");
        }

        // create audio frame buffer
        aframe_buffer = (uint8_t*)malloc(format_desc_.st30_frame_size);

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

        if(aframe_buffer) free(aframe_buffer);

        if(vframe_buffer) free(vframe_buffer);
    }

    /**
     * @brief start output stream handle
     * 
     */
    void decklink_output::start()
    {
        output::start();
        abort_ = false;
        logger->info("decklink_output start, device_index={} pixel_format={} format_desc={}", decklink_index_, pixel_format_string_, format_desc_.name);
        /*
        decklink_thread_ = std::make_unique<std::thread>(std::thread([&](){
            while(!abort_)
            {
                HRESULT result;
                auto frm = get_video_frame();

                // convert frame format
                // sws_scale(sws_ctx_, (const uint8_t *const *)frm->data,
                //                         frm->linesize, 0, format_desc_.height, dst_frame_->data, dst_frame_->linesize);
                // convert uyvy422 10bit to decklink bgra 8 bit
               auto bgra_frame = ycbcr422_to_bgra(frm->data[0], format_desc_);

                // fill playback frame
                uint8_t* deckLinkBuffer = nullptr;
                if (playbackFrame_->GetBytes((void**)&deckLinkBuffer) != S_OK)
                {
                    logger->error("Could not get DeckLinkVideoFrame buffer pointer");
                    continue;
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
        }));
        */
    }
        
    /**
     * @brief stop output stream handle
     * 
     */
    void decklink_output::stop()
    {
        abort_ = true;
        if (decklink_thread_->joinable()) {
            decklink_thread_->join();
        }
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

    /**
     * @brief push a video frame into this output stream buffer
     * 
     */
    void decklink_output::set_video_frame(std::shared_ptr<AVFrame> vframe)
    {
        std::lock_guard<std::mutex> lock(vframe_mutex_);
        if(vframe_buffer_.size() >= vframe_capacity_)
        {
            auto f = vframe_buffer_[0];
            frame_buffer_.pop_front(); // discard the last frame
            //logger->warn("The video frame be discarded");
        }
        vframe_buffer_.push_back(vframe);
        vframe_cv_.notify_all();
    }

    /**
     * @brief push a audio frame into this output stream buffer
     * 
     */
    void decklink_output::set_audio_frame(std::shared_ptr<AVFrame> aframe)
    {
        std::lock_guard<std::mutex> lock(aframe_mutex_);
        if(aframe_buffer_.size() >= aframe_capacity_)
        {
            auto f = aframe_buffer_[0];
            aframe_buffer_.pop_front(); // discard the last frame
            //logger->error("The audio frame be discarded");
        }
        aframe_buffer_.push_back(aframe);
        aframe_cv_.notify_all();
    }

    /**
     * @brief Get a video frame from this output stream buffer
     * 
     */
    std::shared_ptr<AVFrame> decklink_output::get_video_frame()
    {
        std::shared_ptr<AVFrame> vframe;
        std::unique_lock<std::mutex> lock(vframe_mutex_);
        vframe_cv_.wait(lock, [this](){return !vframe_buffer_.empty();});
        vframe = vframe_buffer_[0];
        vframe_buffer_.pop_front();
        return vframe;
    }

    /**
     * @brief Get a audio frame from this output stream buffer
     * 
     */
    std::shared_ptr<AVFrame> decklink_output::get_audio_frame()
    {
        std::shared_ptr<AVFrame> aframe;
        std::unique_lock<std::mutex> lock(aframe_mutex_);
        aframe_cv_.wait(lock, [this](){return !aframe_buffer_.empty();});
        aframe = aframe_buffer_[0];
        aframe_buffer_.pop_front();
        return aframe;
    }

    /**
     * @brief push a video frame into this output stream
     * 
     */
    void decklink_output::display_video_frame(uint8_t* vframe)
    {
        HRESULT result;
        ////////////do not need format convert
        // fill playback frame
        // uint8_t* deckLinkBuffer = nullptr;
        // if (playbackFrame_->GetBytes((void**)&yuvBuffer) != S_OK)
        // {
        //     logger->error("Could not get DeckLinkVideoFrame buffer pointer");
        // }
        // // // deckLinkBuffer = bgra_frame->begin();
        // memset(deckLinkBuffer, 0, playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());
        // memcpy(deckLinkBuffer, vframe, playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());

        ///////////bmdFormat10BitYUV to bmdFormat8BitBGRA
        // fill playback frame
        uint8_t* yuvBuffer = nullptr;
        if (yuv10Frame_->GetBytes((void**)&yuvBuffer) != S_OK)
        {
            logger->error("Could not get DeckLinkVideoFrame buffer pointer");
            return;
        }
        memset(yuvBuffer, 0, yuv10Frame_->GetRowBytes() * yuv10Frame_->GetHeight());
        memcpy(yuvBuffer, vframe, yuv10Frame_->GetRowBytes() * yuv10Frame_->GetHeight());

        if (pixel_format_ == bmdFormat10BitYUV) {
            result = output_->DisplayVideoFrameSync(yuv10Frame_);
        } else {
            
            if (frameConverter->ConvertFrame(yuv10Frame_, playbackFrame_) != S_OK)
            {
                logger->error("Could not get DeckLinkVideoFrame buffer pointer");
                return;
            }

            result = output_->DisplayVideoFrameSync(playbackFrame_);
        }
        display_frame_count++;

        
        if (result != S_OK)
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
            logger->error("Unable to display video output");
        }
    }

    /**
     * @brief push a audio frame into this output stream
     * 
     */
    void decklink_output::display_audio_frame(uint8_t* aframe)
    {
        HRESULT result;
        uint32_t n;
        result = output_->WriteAudioSamplesSync(aframe, format_desc_.sample_num, &n);
        if (result != S_OK)
        {
            logger->error("Unable to display audio output");
        }
    }

    void decklink_output::dump_stat() {
        logger->info("decklink_output device_index={} display_frame_count={}", decklink_index_, display_frame_count);

        display_frame_count = 0;
    }
}