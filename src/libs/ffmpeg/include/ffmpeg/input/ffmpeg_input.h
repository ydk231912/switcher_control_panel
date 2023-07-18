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
  #include <SDL2/SDL.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
 #include <libswresample/swresample.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
}

#include "core/stream/input.h"
#include "core/video_format.h"
#include "core/util/buffer.h"

using namespace seeder::core;
namespace seeder::ffmpeg
{
    class ffmpeg_input : public core::input
    {
      public:
        explicit ffmpeg_input(const std::string &source_id, std::string filename);
        explicit ffmpeg_input(const std::string &source_id, std::string filename, core::video_format_desc& format_desc);
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
        void do_stop();

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

        void set_audio_frame_slice(std::shared_ptr<buffer> asframe);
        std::shared_ptr<buffer> get_audio_frame_slice();

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

        // audio slice buffer
        std::mutex asframe_mutex_;
        std::deque<std::shared_ptr<buffer>> asframe_buffer_;
        const size_t asframe_capacity_ = 1600; // 1 audio frame(p25:40ms) slice to 125us a frame, slices = 40ms / 0.125ms
        std::shared_ptr<buffer> last_asframe_;
        std::condition_variable asframe_cv_;


        std::unique_ptr<std::thread> ffmpeg_thread_ = nullptr;

        // pixel format convert
        SwsContext* sws_ctx_;
        SwrContext* swrCtx;
        core::video_format_desc format_desc_;
        int buf_size_;

        //for test
        std::deque<std::shared_ptr<AVFrame>> avframe_buffer_;

    };
}