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
#include <thread>

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
        void set_video_frame(std::shared_ptr<AVFrame> vframe);
        void set_audio_frame(std::shared_ptr<AVFrame> aframe);

        /**
         * @brief Get a frame from this output stream buffer
         * 
         */
        std::shared_ptr<core::frame> get_frame();
        std::shared_ptr<AVFrame> get_video_frame();
        std::shared_ptr<AVFrame> get_audio_frame();
        /**
         * @brief push a video frame into this output stream
         * 
         */
        void display_video_frame(uint8_t* vframe);

        /**
         * @brief push a audio frame into this output stream
         * 
         */
        void display_audio_frame(uint8_t* aframe);

      private:
        int decklink_index_;
        core::video_format_desc format_desc_;
        BMDVideoInputFlags video_flags_;
        BMDPixelFormat pixel_format_ = bmdFormat8BitBGRA;//bmdFormat8BitBGRA;bmdFormat10BitYUV
        BMDDisplayMode bmd_mode_;
        IDeckLinkDisplayMode* display_mode_;
        IDeckLink* decklink_;
        IDeckLinkOutput* output_;
        bool abort_ = false;

        IDeckLinkMutableVideoFrame*	playbackFrame_ = nullptr;
        IDeckLinkMutableVideoFrame*	yuv10Frame_ = nullptr;
        SwsContext* sws_ctx_ = nullptr;
        AVFrame* dst_frame_ = nullptr;
        uint8_t * dst_frame_buffer_ = nullptr;

        //buffer
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<core::frame>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<core::frame> last_frame_;
        std::condition_variable frame_cv_;

        std::unique_ptr<std::thread> decklink_thread_;

        // video buffer
        std::mutex vframe_mutex_;
        std::deque<std::shared_ptr<AVFrame>> vframe_buffer_;
        const size_t vframe_capacity_ = 5;
        std::shared_ptr<frame> last_vframe_;
        std::condition_variable vframe_cv_;

        // audio buffer
        std::mutex aframe_mutex_;
        std::deque<std::shared_ptr<AVFrame>> aframe_buffer_;
        const size_t aframe_capacity_ = 5;
        std::shared_ptr<frame> last_aframe_;
        std::condition_variable aframe_cv_;

    };
}