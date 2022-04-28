/**
 * @file sdl_consumer.h
 * @author your name (you@domain.com)
 * @brief sdl consumer, display the frame stream in window
 * @version 0.1
 * @date 2022-04-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <deque>

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

namespace seeder::sdl
{
    class sdl_consumer
    {
      public:
        sdl_consumer();
        ~sdl_consumer();

        int start();

        void send_frame(std::shared_ptr<AVFrame> frm);
        std::shared_ptr<AVFrame> get_frame();

        void init();

      private:
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<AVFrame>> frame_buffer_;
        
        SDL_Window* screen;
        SDL_Renderer* sdl_renderer;
        SDL_Texture* sdl_texture;
        SDL_Rect sdl_rect;

        const int width = 960;
        const int height = 540; 

        AVFrame* frm_yuv;
        uint8_t* buffer;
        SwsContext* sws_ctx;

        bool init_flag = false;
    };
}