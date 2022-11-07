/**
 * @file ffmpeg_output.h
 * @author 
 * @brief ffmpeg video player
 * @version 1.0
 * @date 2022-07-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <string>
#include <mutex>
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

#include "core/stream/input.h"
#include "core/video_format.h"

namespace seeder::ffmpeg
{
    class ffmpeg_input : public core::input
    {
      public:
        ffmpeg_input(std::string filename);
        ffmpeg_input(std::string filename, core::video_format_desc& format_desc);
        ~ffmpeg_input();

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

        void run();

        /**
         * @brief push a frame into this input stream
         * 
         */
        void set_frame(std::shared_ptr<core::frame> frm);
        void set_video_frame(std::shared_ptr<AVFrame> vframe);
        void set_audio_frame(std::shared_ptr<AVFrame> aframe);


        /**
         * @brief Get a frame from this input stream
         * 
         */
        std::shared_ptr<core::frame> get_frame();
        std::shared_ptr<AVFrame> get_video_frame();
        std::shared_ptr<AVFrame> get_audio_frame();


        // test
        void set_avframe(std::shared_ptr<AVFrame> frm);
        std::shared_ptr<AVFrame> get_avframe();

      private:
        bool abort = false;
        std::string filename_;

        // buffer
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<core::frame>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<core::frame> last_frame_;
        std::condition_variable frame_cv_;

        // video buffer
        std::mutex vframe_mutex_;
        std::deque<std::shared_ptr<AVFrame>> vframe_buffer_;
        const size_t vframe_capacity_ = 5;
        std::shared_ptr<AVFrame> last_vframe_;
        std::condition_variable vframe_cv_;

        // audio buffer
        std::mutex aframe_mutex_;
        std::deque<std::shared_ptr<AVFrame>> aframe_buffer_;
        const size_t aframe_capacity_ = 5;
        std::shared_ptr<AVFrame> last_aframe_;
        std::condition_variable aframe_cv_;

        std::unique_ptr<std::thread> ffmpeg_thread_ = nullptr;

        // pixel format convert
        SwsContext* sws_ctx_;
        core::video_format_desc format_desc_;
        int buf_size_;

        //for test
        std::deque<std::shared_ptr<AVFrame>> avframe_buffer_;

    };
}