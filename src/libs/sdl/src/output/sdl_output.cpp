/**
 * @file sdl_output.cpp
 * @author 
 * @brief display stream frame to windows
 * @version 1.0
 * @date 2022-07-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "sdl/output/sdl_output.h"
#include "core/util/logger.h"

using namespace seeder::core;
namespace seeder::sdl
{
    sdl_output::sdl_output()
    {
        init();
    }

    sdl_output::sdl_output(core::video_format_desc format_desc)
    :format_desc_(format_desc)
    {
        init();
    }

    sdl_output::~sdl_output()
    {
        if(frm_yuv_)
            av_frame_free(&frm_yuv_);
        if(buffer_)
            av_free(buffer_);
        
        if(sws_ctx_)
            sws_freeContext(sws_ctx_);
    }

    /**
     * @brief initialize sdl 
     * 
     */
    void sdl_output::init()
    {
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
        {
            logger->error("SDL_Init() failed: {}", SDL_GetError());
            throw std::runtime_error("SDL_Init() failed!");
        }

        // SDL 
        screen_ = SDL_CreateWindow("SDL Window",
                                            SDL_WINDOWPOS_UNDEFINED,
                                            SDL_WINDOWPOS_UNDEFINED,
                                            window_width_,
                                            window_height_,
                                            SDL_WINDOW_OPENGL);
        if(screen_ == NULL)
        {
            logger->error("SDL_CreateWindow() is failed: {}", SDL_GetError());
            throw std::runtime_error("SDL_CreateWindow() failed!");
        }

        sdl_renderer_ = SDL_CreateRenderer(screen_, -1, 0);

        SDL_SetRenderDrawColor(sdl_renderer_, 255,0,0,255);
        SDL_RenderClear(sdl_renderer_);
        SDL_RenderPresent(sdl_renderer_);

        sdl_texture_ = SDL_CreateTexture(sdl_renderer_, SDL_PIXELFORMAT_IYUV,
                                                    SDL_TEXTUREACCESS_STREAMING, window_width_, window_height_);
                                                    
        sdl_rect_.x = 0;
        sdl_rect_.y = 0;
        sdl_rect_.w = window_width_;
        sdl_rect_.h = window_height_;

        frm_yuv_ = av_frame_alloc();
        int buf_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, window_width_, 
                                                window_height_,1);
        buffer_ = (uint8_t*)av_malloc(buf_size);
        av_image_fill_arrays(frm_yuv_->data, frm_yuv_->linesize, buffer_, AV_PIX_FMT_YUV420P, 
                            window_width_, window_height_, 1);

        sws_ctx_ = sws_getContext(format_desc_.width, format_desc_.height, AV_PIX_FMT_YUV422P,//AV_PIX_FMT_UYVY422,//(AVPixelFormat)format_desc_.format,
                        window_width_, window_height_, AV_PIX_FMT_YUV420P, 
                        SWS_BICUBIC, NULL, NULL, NULL);
    }

    /**
     * @brief start output stream handle
     * 
     */
    void sdl_output::start()
    {
        abort = false;
        sdl_thread_ = std::make_unique<std::thread>(std::thread([&](){
            while(!abort)
            {
                auto frm = get_avframe();
                if(!frm)
                    continue;

                if(ffmpeg_format_ == AV_PIX_FMT_NONE)
                {
                    ffmpeg_format_ = (AVPixelFormat)frm->format;
                    sws_ctx_ = sws_getContext(format_desc_.width, format_desc_.height, ffmpeg_format_,//AV_PIX_FMT_UYVY422,//(AVPixelFormat)format_desc_.format,
                        window_width_, window_height_, AV_PIX_FMT_YUV420P, 
                        SWS_BICUBIC, NULL, NULL, NULL);
                }

                sws_scale(sws_ctx_, (const uint8_t *const *)frm->data,
                                    frm->linesize, 0, frm->height, frm_yuv_->data, frm_yuv_->linesize);
                
                SDL_UpdateYUVTexture(sdl_texture_, &sdl_rect_, frm_yuv_->data[0], frm_yuv_->linesize[0],
                                    frm_yuv_->data[1], frm_yuv_->linesize[1], frm_yuv_->data[2], frm_yuv_->linesize[2]);

                SDL_RenderClear(sdl_renderer_);
                SDL_RenderCopy(sdl_renderer_, sdl_texture_, NULL, &sdl_rect_);
                SDL_RenderPresent(sdl_renderer_);
            }
        }));
    }
        
    /**
     * @brief stop output stream handle
     * 
     */
    void sdl_output::stop()
    {
        abort = true;
    }

    /**
     * @brief push a frame into this output stream buffer
     * 
     */
    void sdl_output::set_frame(std::shared_ptr<core::frame> frm)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_cv_.wait(lock, [this](){return frame_buffer_.size() < frame_capacity_;}); // block until the buffer is not empty
        frame_buffer_.push_back(frm);
        frame_cv_.notify_all();
    }

    /**
     * @brief Get a frame from this output stream buffer
     * 
     */
    std::shared_ptr<core::frame> sdl_output::get_frame()
    {
        std::shared_ptr<core::frame> frm = nullptr;
        
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_cv_.wait(lock, [this](){return !(frame_buffer_.empty());}); // block until the buffer is not empty
        frm = frame_buffer_[0];
        frame_buffer_.pop_front();
        frame_cv_.notify_all();

        return frm;
    }

    void sdl_output::set_avframe(std::shared_ptr<AVFrame> frm)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_cv_.wait(lock, [this](){return avframe_buffer_.size() < frame_capacity_;}); // block until the buffer is not empty
        avframe_buffer_.push_back(frm);
        frame_cv_.notify_all();
    }

    std::shared_ptr<AVFrame> sdl_output::get_avframe()
    {
        std::shared_ptr<AVFrame> frm = nullptr;
        
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_cv_.wait(lock, [this](){return !(avframe_buffer_.empty());}); // block until the buffer is not empty
        frm = avframe_buffer_[0];
        avframe_buffer_.pop_front();
        frame_cv_.notify_all();

        return frm;
    }
}