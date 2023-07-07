

#include <unistd.h>
#include <iostream>

#include "ffmpeg/input/ffmpeg_input.h"
#include "core/util/logger.h"
#include "core/util/timer.h"

using namespace seeder::core;
namespace seeder::ffmpeg
{
    ffmpeg_input::ffmpeg_input(const std::string &source_id, std::string filename)
    :input(source_id), filename_(filename)
    {
    }

    ffmpeg_input::ffmpeg_input(const std::string &source_id, std::string filename, core::video_format_desc& format_desc)
    :input(source_id), filename_(filename),
    format_desc_(format_desc)
    {
        

    }

    ffmpeg_input::~ffmpeg_input()
    {
        if(!sws_ctx_) sws_freeContext(sws_ctx_);
    }

    /**
     * @brief start output stream handle
     * 
     */
    void ffmpeg_input::start()
    {
        input::start();
        abort = false;

        ffmpeg_thread_ = std::make_unique<std::thread>(std::thread([&](){
            while(!abort) // loop playback
            {
               run();
            }
       }));
    }
        
    /**
     * @brief stop output stream handle
     * 
     */
    void ffmpeg_input::stop()
    {
        abort = true;
    }

    void ffmpeg_input::run()
    {
        
        int ret;
        
        AVFormatContext* fmt_ctx = NULL;
        ret = avformat_open_input(&fmt_ctx, filename_.data(), NULL, NULL);
        if(ret!=0)
        {
            logger->error("avformat_open_input() failed");
            throw std::runtime_error("avformat_open_input() failed");
        }

        ret =avformat_find_stream_info(fmt_ctx, NULL);
        if(ret!=0)
        {
            logger->error("avformat_find_stream_info() failed");
            throw std::runtime_error("avformat_find_stream_info() failed");
        }

        int v_idx = -1;
        int a_idx = -1;
        
    

        for(uint i=0; i<fmt_ctx->nb_streams; i++)
        {
            if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                v_idx = i;
                break;
            }
        }

        for(uint i=0; i<fmt_ctx->nb_streams; i++)
        {
            if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                a_idx = i;
                break;
            }
        }

    
        if(v_idx == -1)
        {
            logger->error("Can not find a video stream");
            throw std::runtime_error("Can not find a video stream");
        }



        AVCodecParameters* codec_par = fmt_ctx->streams[v_idx]->codecpar;
        AVCodecParameters* codec_par_a = fmt_ctx->streams[a_idx]->codecpar;

        AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
        AVCodec* codec_a = avcodec_find_decoder(codec_par_a->codec_id);
        if(codec == NULL)
        {
            logger->error("Can not find video codec");
            throw std::runtime_error("Can not find video codec");
        }

        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        AVCodecContext* codec_ctx_a = avcodec_alloc_context3(codec_a);
        ret = avcodec_parameters_to_context(codec_ctx, codec_par);
        if(ret < 0)
        {
            logger->error("avcodec_parameters_to_context() failed");
            throw std::runtime_error("avcodec_parameters_to_context() failed");
        }
        ret = avcodec_parameters_to_context(codec_ctx_a, codec_par_a);
        if(ret < 0)
        {
            logger->error("avcodec_parameters_to_context() failed");
            throw std::runtime_error("avcodec_parameters_to_context() failed");
        }
        av_opt_set_int(codec_ctx, "threads", 12, 0);
        
        ret = avcodec_open2(codec_ctx, codec, NULL);
        if(ret < 0)
        {
            logger->error("avcodec_open2() failed {}", ret);
            throw std::runtime_error("avcodec_open2() failed");
        }
        ret = avcodec_open2(codec_ctx_a, codec_a, NULL);
        if(ret < 0)
        {
            logger->error("avcodec_open2() failed {}", ret);
            throw std::runtime_error("avcodec_open2() failed");
        }
        printf("%s\n",codec_a->name);
        printf("%d\n",codec_ctx_a->channels);
        printf("%d\n",codec_ctx_a->sample_rate);
        printf("%d\n",codec_ctx_a->sample_fmt);

        AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
        while(1)
        {
           
            timer t1;
            if(av_read_frame(fmt_ctx, packet) != 0) break;

            if(packet->stream_index == v_idx)
            {
                ret =  avcodec_send_packet(codec_ctx, packet);
                if(ret != 0)
                {
                    logger->error("avcodec_send_packet() failed {}", ret);
                    throw std::runtime_error("avcodec_send_packet() failed");
                }

                //auto avframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
                auto vframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });

                ret = avcodec_receive_frame(codec_ctx, vframe.get()); //avframe.get()
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    continue;
                }
                if(ret != 0)
                {
                    logger->error("avcode_receive_frame() failed {}", ret);
                    throw std::runtime_error("avcode_receive_frame() failed");
                }

                if(!sws_ctx_)
                {
                    buf_size_ = av_image_get_buffer_size(AV_PIX_FMT_YUV422P10LE, format_desc_.width, 
                                                            format_desc_.height, 1);
                    sws_ctx_ = sws_getContext(format_desc_.width, format_desc_.height, (AVPixelFormat)vframe->format,//AV_PIX_FMT_UYVY422,//(AVPixelFormat)format_desc_.format,
                                    format_desc_.width, format_desc_.height, AV_PIX_FMT_YUV422P10LE, SWS_BICUBIC, NULL, NULL, NULL);
                }

                if(vframe->format != AV_PIX_FMT_YUV422P10LE)
                {
                    // convert pixel format
                    auto f = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr){av_frame_free(&ptr);});
                    f->width = format_desc_.width;
                    f->height = format_desc_.height;
                    f->format = AV_PIX_FMT_YUV422P10LE;
                    av_frame_get_buffer(f.get(), 1);
                    sws_scale(sws_ctx_, (const uint8_t *const *)vframe->data,  //frm->data,
                                    vframe->linesize, 0, vframe->height, f->data, f->linesize);
                    set_video_frame(f);
                }
                else
                {
                    set_video_frame(vframe);
                }

                //std::cout << "ffmpeg decode 1 frame : " << t1.elapsed() << std::endl;
                //auto frm = std::make_shared<core::frame>();
                //frm->set_video_data(avframe);
                //frm->video = avframe;
                //set_frame(frm);
                //logger->info("frame buffer number {}", frame_buffer_.size());
                //set_avframe(avframe);
            }
            if(packet->stream_index == a_idx)
            {
                auto avframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
			   // ret = avcodec_decode_audio4( codec_ctx_a, avframe.get(),&got_picture, packet);


                // ret =  avcodec_send_packet(codec_ctx_a, packet);  
                // if(ret != 0)
                // {
                //     logger->error("avcodec_send_packet() failed {}", ret);
                //     throw std::runtime_error("avcodec_send_packet() failed");
                // }


  
                
                // ret = avcodec_receive_frame(codec_ctx_a, avframe.get()); //avframe.get()
                // if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                // {
                //     continue;
                // }
                // if(ret != 0)
                // {
                //     logger->error("avcode_receive_frame() failed {}", ret);
                //     throw std::runtime_error("avcode_receive_frame() failed");
                // }

                 set_audio_frame(avframe);
            }


            av_packet_unref(packet);
        }

        avcodec_close(codec_ctx);
        avcodec_close(codec_ctx_a);
        avformat_close_input(&fmt_ctx);
    }

    /**
     * @brief push a frame into this output stream
     * 
     */
    void ffmpeg_input::set_frame(std::shared_ptr<core::frame> frm)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_cv_.wait(lock, [this](){return frame_buffer_.size() < frame_capacity_;}); // block until the buffer is not full
        frame_buffer_.push_back(frm);
        frame_cv_.notify_all();
    }

    /**
     * @brief push a video frame into this output stream
     * 
     */
    void ffmpeg_input::set_video_frame(std::shared_ptr<AVFrame> vframe)
    {
        std::unique_lock<std::mutex> lock(vframe_mutex_);
        vframe_cv_.wait(lock, [this](){return vframe_buffer_.size() < vframe_capacity_;}); // block until the buffer is not full
        vframe_buffer_.push_back(vframe);
        vframe_cv_.notify_all();
    }

    /**
     * @brief push a audio frame into this output stream
     * 
     */
    void ffmpeg_input::set_audio_frame(std::shared_ptr<AVFrame> aframe)
    {
        std::unique_lock<std::mutex> lock(aframe_mutex_);
        aframe_cv_.wait(lock, [this](){return aframe_buffer_.size() < aframe_capacity_;}); // block until the buffer is not full
        aframe_buffer_.push_back(aframe);
        aframe_cv_.notify_all();
    }

    /**
     * @brief Get a video frame from this output stream
     * 
     */
    std::shared_ptr<AVFrame> ffmpeg_input::get_video_frame()
    {
        std::shared_ptr<AVFrame> vframe;
        {
            std::unique_lock<std::mutex> lock(vframe_mutex_);

            if(vframe_buffer_.size() > 0)
            {
                vframe = vframe_buffer_[0];
                vframe_buffer_.pop_front();
                vframe_cv_.notify_all();
            }
        }

        return vframe;
    }
    
    /**
     * @brief Get a audio frame from this output stream
     * 
     */
    std::shared_ptr<AVFrame> ffmpeg_input::get_audio_frame()
    {
        std::shared_ptr<AVFrame> aframe;
        {
            std::unique_lock<std::mutex> lock(aframe_mutex_);

            if(aframe_buffer_.size() > 0)
            {
                aframe = aframe_buffer_[0];
                aframe_buffer_.pop_front();
                aframe_cv_.notify_all();
            }
        }

        return aframe;
    }

    // /**
    //  * @brief Get a frame from this output stream
    //  * 
    //  */
    std::shared_ptr<core::frame> ffmpeg_input::get_frame()
    {
        std::shared_ptr<core::frame> frm;
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            // if(frame_buffer_.size() < 1)
            //     return last_frame_;
            // frame_cv_.wait(lock, [this](){return !(frame_buffer_.empty());}); 
            // frm = frame_buffer_[0];
            // frame_buffer_.pop_front();
            // last_frame_ = frm;

            if(frame_buffer_.size() > 0)
            {
                frm = frame_buffer_[0];
                frame_buffer_.pop_front();
                frame_cv_.notify_all();
            }
        }

        return frm;
    }

     /**
     * @brief push the audio frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void ffmpeg_input::set_audio_frame_slice(std::shared_ptr<buffer> asframe)
    {
        std::lock_guard<std::mutex> lock(asframe_mutex_);
        if(asframe_buffer_.size() >= asframe_capacity_)
        {
            auto f = asframe_buffer_[0];
            asframe_buffer_.pop_front(); // discard the oldest frame
            logger->error("{}, The audio frame is discarded", __func__);
        }
        asframe_buffer_.push_back(asframe);
        asframe_cv_.notify_all();
    }

     /**
     * @brief Get a audio frame slice from this input stream
     * 
     */
    std::shared_ptr<buffer> ffmpeg_input::get_audio_frame_slice()
    {
        std::shared_ptr<buffer> asframe;
        std::unique_lock<std::mutex> lock(asframe_mutex_);
        if(asframe_buffer_.size() > 0)
        {
            asframe = asframe_buffer_[0];
            asframe_buffer_.pop_front();
            return asframe;
        }
        asframe_cv_.wait(lock, [this](){return !(asframe_buffer_.empty());}); // block until the buffer is not empty
        asframe = asframe_buffer_[0];
        asframe_buffer_.pop_front();
        return asframe;
    }

    // void ffmpeg_input::set_avframe(std::shared_ptr<AVFrame> frm)
    // {
    //     std::unique_lock<std::mutex> lock(frame_mutex_);
    //     frame_cv_.wait(lock, [this](){return avframe_buffer_.size() < frame_capacity_;}); // block until the buffer is not full
    //     avframe_buffer_.push_back(frm);
    //     frame_cv_.notify_all();
    // }

    // std::shared_ptr<AVFrame> ffmpeg_input::get_avframe()
    // {
    //     std::shared_ptr<AVFrame> frm;
    //     {
    //         std::unique_lock<std::mutex> lock(frame_mutex_);
    //         // if(frame_buffer_.size() < 1)
    //         //     return last_frame_;
    //         // frame_cv_.wait(lock, [this](){return !(frame_buffer_.empty());}); 
    //         // frm = frame_buffer_[0];
    //         // frame_buffer_.pop_front();
    //         // last_frame_ = frm;

    //         if(avframe_buffer_.size() > 0)
    //         {
    //             frm = avframe_buffer_[0];
    //             avframe_buffer_.pop_front();
    //             frame_cv_.notify_all();
    //         }
    //     }

    //     return frm;
    // }
}