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

#pragma once

#include <mutex>
#include <memory>
#include <deque>
#include <condition_variable>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "../interop/DeckLinkAPI.h"
#include "core/video_format.h"
#include "core/util/buffer.h"

using namespace seeder::core;
namespace seeder::decklink {
    class decklink_consumer
    {
      public:
        /**
         * @brief Construct a new decklink consumer::decklink consumer object.
         * initialize deckllink device and start output stream
         */
        decklink_consumer(int device_id, core::video_format_desc& format_desc);
        ~decklink_consumer();

        /**
         * @brief start decklink output
         * 
         */
        void start();

        /**
         * @brief stop decklink output
         * 
         */
        void stop();

        /**
         * @brief send the frame object to the frame_buffer
         * 
         * @return void
         */
        void send_frame(std::shared_ptr<buffer>);

      private:
        int decklink_index_;
        core::video_format_desc format_desc_;
        BMDVideoInputFlags video_flags_;
        BMDPixelFormat pixel_format_ = bmdFormat8BitBGRA;
        BMDDisplayMode bmd_mode_;
        IDeckLinkDisplayMode* display_mode_;
        IDeckLink* decklink_;
        IDeckLinkOutput* output_;
        bool abort_ = false;

        IDeckLinkMutableVideoFrame*	playbackFrame_ = nullptr;
        SwsContext* sws_ctx_ = nullptr;
        AVFrame* dst_frame_ = nullptr;
        uint8_t * dst_frame_buffer_ = nullptr;

        //buffer
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<buffer>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<buffer> last_frame_;
        std::condition_variable frame_cv_;
    };

}