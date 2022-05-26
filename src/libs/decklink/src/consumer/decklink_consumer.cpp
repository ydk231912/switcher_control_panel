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

        // create display frame
        auto outputBytesPerRow = ((display_mode_->GetWidth() + 47) / 48) * 128; // bmdFormat10BitYUV
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
    }
    
    decklink_consumer::~decklink_consumer()
    {
        if(display_mode_ != nullptr) display_mode_->Release();

        if(output_ != nullptr) output_->Release();

        if(decklink_ != nullptr) decklink_->Release();

        if(playbackFrame_ != nullptr) playbackFrame_->Release();
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
            std::shared_ptr<AVFrame> frame;
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

            // fill playback frame
            uint8_t* deckLinkBuffer = nullptr;
            if (playbackFrame_->GetBytes((void**)&deckLinkBuffer) != S_OK)
            {
                logger->error("Could not get DeckLinkVideoFrame buffer pointer");
            }
            //memset(deckLinkBuffer, 0, playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());
            //memcpy(deckLinkBuffer, frame->data[0], playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());

            deckLinkBuffer = reinterpret_cast<uint8_t*>(frame->data[0]);

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
    void decklink_consumer::send_frame(std::shared_ptr<AVFrame> frame)
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