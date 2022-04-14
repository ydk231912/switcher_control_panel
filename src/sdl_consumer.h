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

namespace seeder
{
    class sdl_consumer
    {
      public:
        sdl_consumer();
        ~sdl_consumer();

        int handler(std::shared_ptr<AVFrame> frame);

        void push_frame(std::shared_ptr<AVFrame> frm);
        std::shared_ptr<AVFrame> receive_frame();


        void init();

        // for test
        int play(std::string filename);


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