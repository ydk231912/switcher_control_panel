/**
 * @file decklink_output.h
 * @author
 * @brief decklink output, send SDI frame by decklink card
 * @version 1.0
 * @date 2022-07-29
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
#include "core/stream/output.h"

using namespace seeder::core;
namespace seeder::decklink
{
    class decklink_output : public core::output
    {
      public:
        decklink_output(int device_id, core::video_format_desc format_desc);
        ~decklink_output();

        /**
         * @brief start output stream handle
         * 
         */
        void start();
            
        /**
         * @brief stop output stream handle
         * 
         */
        void stop();

        /**
         * @brief push a frame into this output stream buffer
         * 
         */
        void set_frame(std::shared_ptr<core::frame> frm);

        /**
         * @brief Get a frame from this output stream buffer
         * 
         */
        std::shared_ptr<core::frame> get_frame();

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
        std::deque<std::shared_ptr<core::frame>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<core::frame> last_frame_;
        std::condition_variable frame_cv_;

    };
}