#include <iostream>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>


#include "sdl_consumer.h"


namespace seeder 
{
    sdl_consumer::sdl_consumer()
    {
        // if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
        // {
        //     printf("SDL_Init() failed: %s \n", SDL_GetError());
        // }

        // // SDL 
        // screen = SDL_CreateWindow(title.data(),
        //                                     SDL_WINDOWPOS_UNDEFINED,
        //                                     SDL_WINDOWPOS_UNDEFINED,
        //                                     width,
        //                                     height,
        //                                     SDL_WINDOW_OPENGL);
        // if(screen == NULL)
        // {
        //     printf("SDL_CreateWindow() is failed: %s\n", SDL_GetError());
        // }

        // sdl_renderer = SDL_CreateRenderer(screen, -1, 0);

        // SDL_SetRenderDrawColor(sdl_renderer, 255,0,0,255);
        // SDL_RenderClear(sdl_renderer);
        // SDL_RenderPresent(sdl_renderer);

        // sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_IYUV,
        //                                             SDL_TEXTUREACCESS_STREAMING, width, height);
                                                    
        // sdl_rect.x = 0;
        // sdl_rect.y = 0;
        // sdl_rect.w = width;
        // sdl_rect.h = height;

        // frm_yuv = av_frame_alloc();
        // int buf_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, 
        //                                         height,1);
        // buffer = (uint8_t*)av_malloc(buf_size);
        // av_image_fill_arrays(frm_yuv->data, frm_yuv->linesize, buffer, AV_PIX_FMT_YUV420P, 
        //                     width, height, 1);

        // sws_ctx = sws_getContext(1920, 1080, AV_PIX_FMT_YUV422P,
        //                 width, height, AV_PIX_FMT_YUV420P, 
        //                 SWS_BICUBIC, NULL, NULL, NULL);
        
    }

    void sdl_consumer::init()
    {
        
    }

    sdl_consumer::~sdl_consumer()
    {
        av_frame_free(&frm_yuv);
        av_free(buffer);
        sws_freeContext(sws_ctx);
    }

    int sdl_consumer::handler(std::shared_ptr<AVFrame> frame)
    {
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

    // for test
    int sdl_consumer::play(std::string filename)
    {
        int ret;
        AVFormatContext* fmt_ctx = NULL;
        ret = avformat_open_input(&fmt_ctx, filename.data(), NULL, NULL);
        if(ret!=0)
        {
            printf("avformat_open_input() failed \n");
            return -1;
        }

        ret =avformat_find_stream_info(fmt_ctx, NULL);
        if(ret!=0)
        {
            printf("avformat_find_stream_info() failed \n");
            return -1;
        }

        int v_idx = -1;
        for(int i=0; i<fmt_ctx->nb_streams; i++)
        {
            if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                v_idx = i;
                break;
            }
        }
        if(v_idx == -1)
        {
            printf("Can not find a video stream /n");
            return -1;
        }

        AVCodecParameters* codec_par = fmt_ctx->streams[v_idx]->codecpar;
        AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
        if(codec == NULL)
        {
            printf("Can not find codec \n");
            return -1;
        }

        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        ret = avcodec_parameters_to_context(codec_ctx, codec_par);
        if(ret < 0)
        {
            printf("avcodec_parameters_to_context() failed %d \n", ret);
            return -1;
        }
        
        ret = avcodec_open2(codec_ctx, codec, NULL);
        if(ret < 0)
        {
            printf("avcodec_open2() failed %d \n", ret);
            return -1;
        }

        AVFrame* frame = av_frame_alloc();

        // SwsContext* sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, (AVPixelFormat)codec_ctx->pix_fmt,
        //                                 width, height, AV_PIX_FMT_YUV420P, 
        //                                 SWS_BICUBIC, NULL, NULL, NULL);

        AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
        while(av_read_frame(fmt_ctx, packet) == 0)
        {
            if(packet->stream_index == v_idx)
            {
                ret =  avcodec_send_packet(codec_ctx, packet);
                if(ret != 0)
                {
                    printf("avcodec_send_packet() failed %d \n", ret);
                    return -1;
                }

                ret = avcodec_receive_frame(codec_ctx, frame);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    continue;
                }
                if(ret != 0)
                {
                    printf("avcode_receive_frame() failed %d \n", ret);
                    return -1;
                }

                sws_scale(sws_ctx, (const uint8_t *const *)frame->data,
                                    frame->linesize, 0, codec_ctx->height, frm_yuv->data, frm_yuv->linesize);

                SDL_UpdateYUVTexture(sdl_texture, &sdl_rect, frm_yuv->data[0], frm_yuv->linesize[0],
                                    frm_yuv->data[1], frm_yuv->linesize[1], frm_yuv->data[2], frm_yuv->linesize[2]);

                
                SDL_RenderClear(sdl_renderer);
                SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
                SDL_RenderPresent(sdl_renderer);
                
                SDL_Delay(40);
                
            }
            av_packet_unref(packet);
        }

    }

    void sdl_consumer::push_frame(std::shared_ptr<AVFrame> frm)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_buffer_.push_back(frm);
        lock.unlock();
    }

    std::shared_ptr<AVFrame> sdl_consumer::receive_frame()
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