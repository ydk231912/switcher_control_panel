#include <iostream>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#include "sdl/consumer/sdl_consumer.h"

namespace seeder::sdl
{   
    sdl_consumer::sdl_consumer()
    {
        return;
    }

    sdl_consumer::~sdl_consumer()
    {
        av_frame_free(&frm_yuv);
        av_free(buffer);
        sws_freeContext(sws_ctx);
    }

    void sdl_consumer::init()
    {
        
    }

    int sdl_consumer::start()
    {
        while(true)
        {
            auto frame = get_frame();
            if(!frame)
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
                continue;
            }

            if(!init_flag)
            {
                if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
                {
                    printf("SDL_Init() failed: %s \n", SDL_GetError());
                }

                // SDL 
                screen = SDL_CreateWindow("Simple ffmpeg player's Window",
                                                    SDL_WINDOWPOS_UNDEFINED,
                                                    SDL_WINDOWPOS_UNDEFINED,
                                                    width,
                                                    height,
                                                    SDL_WINDOW_OPENGL);
                if(screen == NULL)
                {
                    printf("SDL_CreateWindow() is failed: %s\n", SDL_GetError());
                }

                sdl_renderer = SDL_CreateRenderer(screen, -1, 0);

                SDL_SetRenderDrawColor(sdl_renderer, 255,0,0,255);
                SDL_RenderClear(sdl_renderer);
                SDL_RenderPresent(sdl_renderer);

                sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_IYUV,
                                                            SDL_TEXTUREACCESS_STREAMING, width, height);
                                                            
                sdl_rect.x = 0;
                sdl_rect.y = 0;
                sdl_rect.w = width;
                sdl_rect.h = height;

                frm_yuv = av_frame_alloc();
                int buf_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, 
                                                        height,1);
                buffer = (uint8_t*)av_malloc(buf_size);
                av_image_fill_arrays(frm_yuv->data, frm_yuv->linesize, buffer, AV_PIX_FMT_YUV420P, 
                                    width, height, 1);

                sws_ctx = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
                                width, height, AV_PIX_FMT_YUV420P, 
                                SWS_BICUBIC, NULL, NULL, NULL);

                init_flag = true;
            }

            sws_scale(sws_ctx, (const uint8_t *const *)frame->data,
                                        frame->linesize, 0, frame->height, frm_yuv->data, frm_yuv->linesize);

            SDL_UpdateYUVTexture(sdl_texture, &sdl_rect, frm_yuv->data[0], frm_yuv->linesize[0],
                                frm_yuv->data[1], frm_yuv->linesize[1], frm_yuv->data[2], frm_yuv->linesize[2]);

            SDL_RenderClear(sdl_renderer);
            SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
            SDL_RenderPresent(sdl_renderer);
        }
    }

    void sdl_consumer::send_frame(std::shared_ptr<AVFrame> frm)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_buffer_.push_back(frm);
        lock.unlock();
    }

    std::shared_ptr<AVFrame> sdl_consumer::get_frame()
    {
        std::shared_ptr<AVFrame> frame = NULL;
        
        if(frame_buffer_.size() > 0)
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            frame = frame_buffer_[0];
            frame_buffer_.pop_front();
            lock.unlock();
        }

        return frame;        
    }
}