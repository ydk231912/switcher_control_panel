#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_render.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

#include "video_play.h"


namespace seeder
{
    video_play::video_play(){
        //sdl_ = sdl_consumer();
    }
    video_play::~video_play(){}

    /**
     * @brief play video, send rtp package by udp
     * 
     */
    int video_play::play(std::string filename)
    {
        int ret;
        int count = 0;
        

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
        for(uint i=0; i<fmt_ctx->nb_streams; i++)
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

        //get system precision time
        int64_t start_time = this->get_timestamp();
        auto fps = (double)1/(fmt_ctx->streams[v_idx]->avg_frame_rate.num/fmt_ctx->streams[v_idx]->avg_frame_rate.den);//duration per frame

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

                auto avframe = frame_alloc();

                ret = avcodec_receive_frame(codec_ctx, avframe.get());
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    continue;
                }
                if(ret != 0)
                {
                    printf("avcode_receive_frame() failed %d \n", ret);
                    return -1;
                }

                // sleep until the queue is not full
                while(frame_buffer_.size() >= buffer_capacity_)
                {
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(40));
                }

                util::frame frm;
                int duration = fps * 1000 * avframe->pts; //the milliseconds from play start time to now
                // st2110 rtp video timestamp = seconds * 90000 mod 2^32
                frm.timestamp = (int)(((start_time + duration) * 90) % (int)pow(2, 32));
                frm.video = std::move(avframe);
                {
                    std::unique_lock<std::mutex> lock(buffer_mutex_);
                    frame_buffer_.push_back(frm);
                    lock.unlock();
                    //debug
                    //std::cout << "video_play::play: frame buffer size: " << frame_buffer_.size() << std::endl;
                }

                // count++;
                // if(count > 3)
                //     break;
            }
            av_packet_unref(packet);
        }

        avcodec_close(codec_ctx);
        avformat_close_input(&fmt_ctx);

        return 0;
    }

    /**
     * @brief get video frame from the buffer
     * 
     * @return frame
     */
    util::frame video_play::receive_frame()
    {
        util::frame frm;
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        if(frame_buffer_.empty())
            return frm;

        frm = frame_buffer_[0];
        frame_buffer_.pop_front();
        lock.unlock();
        return frm;
    }

    /**
     * @brief allocate AVFrame, and package as shared_ptr
     * 
     * @return std::shared_ptr<AVFrame> 
     */

    std::shared_ptr<AVFrame> video_play::frame_alloc()
    {
        const auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { 
            av_frame_free(&ptr); 
            });
        if (!frame)
            std::cout << "frame_alloc error" << std::endl;

        return frame;
    }

    // get system ptp time
    int64_t video_play::get_timestamp()
    {
         // get system current precise time
        std::chrono::milliseconds ms = std::chrono::duration_cast <std::chrono::milliseconds> (
                std::chrono::system_clock::now().time_since_epoch()
                ); //milliseconds since 1970.1.1 0:0:0

        // st2110 rtp video timestamp = seconds * 90000 mod 2^32
        return ms.count();
    }
//}

///////////////////backup

int video_play::sdl_play(std::string filename)
    {
        int ret;
        int count = 0;

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

        //AVFrame* frm_raw = av_frame_alloc();
        AVFrame* frm_yuv = av_frame_alloc();

        auto frm_raw = frame_alloc();

        int buf_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, codec_ctx->width, 
                                                codec_ctx->height,1);
        uint8_t* buffer = (uint8_t*)av_malloc(buf_size);
        av_image_fill_arrays(frm_yuv->data, frm_yuv->linesize, buffer, AV_PIX_FMT_YUV420P, 
                            codec_ctx->width, codec_ctx->height, 1);
        SwsContext* sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                                            codec_ctx->width, codec_ctx->height, AV_PIX_FMT_YUV420P, 
                                            SWS_BICUBIC, NULL, NULL, NULL);

        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
        {
            printf("SDL_Init() failed: %s \n", SDL_GetError());
            return -1;
        }

        // SDL 
        SDL_Window* screen = SDL_CreateWindow("Simple ffmpeg player's Window",
                                            SDL_WINDOWPOS_UNDEFINED,
                                            SDL_WINDOWPOS_UNDEFINED,
                                            codec_ctx->width,
                                            codec_ctx->height,
                                            SDL_WINDOW_OPENGL);
        if(screen == NULL)
        {
            printf("SDL_CreateWindow() is failed: %s\n", SDL_GetError());
            return -1;
        }

        SDL_Renderer* sdl_renderer = SDL_CreateRenderer(screen, -1, 0);
        // SDL_Texture* sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_YV12,
        //                                             SDL_TEXTUREACCESS_STREAMING, codec_ctx->width, codec_ctx->height);
        SDL_Texture* sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_IYUV,
                                                    SDL_TEXTUREACCESS_STREAMING, codec_ctx->width, codec_ctx->height);
                                                    
        SDL_Rect sdl_rect;
        sdl_rect.x = 0;
        sdl_rect.y = 0;
        sdl_rect.w = codec_ctx->width;
        sdl_rect.h = codec_ctx->height;

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

                ret = avcodec_receive_frame(codec_ctx, frm_raw.get());
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    continue;
                }
                if(ret != 0)
                {
                    printf("avcode_receive_frame() failed %d \n", ret);
                    return -1;
                }

                sws_scale(sws_ctx, (const uint8_t *const *)frm_raw->data,
                                    frm_raw->linesize, 0, codec_ctx->height, frm_yuv->data, frm_yuv->linesize);

                SDL_UpdateYUVTexture(sdl_texture, &sdl_rect, frm_yuv->data[0], frm_yuv->linesize[0],
                                    frm_yuv->data[1], frm_yuv->linesize[1], frm_yuv->data[2], frm_yuv->linesize[2]);

                //  SDL_UpdateYUVTexture(sdl_texture, &sdl_rect, frm_raw->data[0], frm_raw->linesize[0],
                //                       frm_raw->data[1], frm_raw->linesize[1], frm_raw->data[2], frm_raw->linesize[2]);

                SDL_RenderClear(sdl_renderer);
                SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
                SDL_RenderPresent(sdl_renderer);
                
                // rtp_->send_rtp(frm_raw);
                
                SDL_Delay(40);
                
                count++;
                //if(count > 5)
                    //break;
            }
            av_packet_unref(packet);
        }

        SDL_Quit();
        sws_freeContext(sws_ctx);
        av_free(buffer);
        av_frame_free(&frm_yuv);
        //av_frame_free(&frm_raw);
        avcodec_close(codec_ctx);
        avformat_close_input(&fmt_ctx);

        return 0;
    }
}
