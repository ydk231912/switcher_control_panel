/**
 * @file sdl_output.h
 * @author 
 * @brief display stream frame to windows
 * @version 1.0
 * @date 2022-07-29
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

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_render.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "core/stream/output.h"
#include "core/video_format.h"

namespace seeder::sdl
{
    class sdl_output : public core::output
    {
      public:
        sdl_output();
        sdl_output(core::video_format_desc format_desc);
        ~sdl_output();

        void init();

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

        // test
        void set_avframe(std::shared_ptr<AVFrame> frm);
        std::shared_ptr<AVFrame> get_avframe();

        /**
         * @brief push a video frame into this output stream
         * 
         */
        void set_video_frame(std::shared_ptr<AVFrame> vframe);

        /**
         * @brief push a audio frame into this output stream
         * 
         */
        void set_audio_frame(std::shared_ptr<AVFrame> aframe);

        /**
         * @brief Get a video frame from this output stream
         * 
         */
        std::shared_ptr<AVFrame> get_video_frame();

        /**
         * @brief Get a audio frame from this output stream
         * 
         */
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
        core::video_format_desc format_desc_;

        bool abort = false;
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<core::frame>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<core::frame> last_frame_;
        std::condition_variable frame_cv_;

        SDL_Window* screen_;
        SDL_Renderer* sdl_renderer_;
        SDL_Texture* sdl_texture_;
        SDL_Rect sdl_rect_;

        const int window_width_ = 960;
        const int window_height_ = 540; 

        AVFrame* frm_yuv_;
        uint8_t* buffer_;
        SwsContext* sws_ctx_;

        std::unique_ptr<std::thread> sdl_thread_ = nullptr;

        AVPixelFormat ffmpeg_format_ = AV_PIX_FMT_NONE;

        // for test
        std::deque<std::shared_ptr<AVFrame>> avframe_buffer_;
    };
}