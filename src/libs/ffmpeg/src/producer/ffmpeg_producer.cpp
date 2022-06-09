/**
 * @file ffmpeg_producer.h
 * @author 
 * @brief ffmpeg video player
 * @version 
 * @date 2022-06-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

#include "producer/ffmpeg_producer.h"
#include "util/logger.h"

using namespace seeder::util;
namespace seeder::ffmpeg
{
    ffmpeg_producer::ffmpeg_producer()
    {
    }

    ffmpeg_producer::~ffmpeg_producer()
    {
    }

    /**
     * @brief play video, send rtp package by udp
     * 
     */
    void ffmpeg_producer::run(std::string filename)
    {
        int ret;
        int count = 0;
        
        AVFormatContext* fmt_ctx = NULL;
        ret = avformat_open_input(&fmt_ctx, filename.data(), NULL, NULL);
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
            logger->error("Can not find a video stream");
            throw std::runtime_error("Can not find a video stream");
        }

        AVCodecParameters* codec_par = fmt_ctx->streams[v_idx]->codecpar;
        AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
        if(codec == NULL)
        {
            logger->error("Can not find video codec");
            throw std::runtime_error("Can not find video codec");
        }

        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        ret = avcodec_parameters_to_context(codec_ctx, codec_par);
        if(ret < 0)
        {
            logger->error("avcodec_parameters_to_context() failed");
            throw std::runtime_error("avcodec_parameters_to_context() failed");
        }
        
        ret = avcodec_open2(codec_ctx, codec, NULL);
        if(ret < 0)
        {
            logger->error("avcodec_open2() failed {}", ret);
            throw std::runtime_error("avcodec_open2() failed");
        }

        AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
        while(av_read_frame(fmt_ctx, packet) == 0)
        {
            if(packet->stream_index == v_idx)
            {
                ret =  avcodec_send_packet(codec_ctx, packet);
                if(ret != 0)
                {
                    logger->error("avcodec_send_packet() failed {}", ret);
                    throw std::runtime_error("avcodec_send_packet() failed");
                }

                auto avframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });

                ret = avcodec_receive_frame(codec_ctx, avframe.get());
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    continue;
                }
                if(ret != 0)
                {
                    logger->error("avcode_receive_frame() failed {}", ret);
                    throw std::runtime_error("avcode_receive_frame() failed");
                }

                // sleep until the queue is not full
                std::unique_lock<std::mutex> lock(frame_mutex_);
                frame_cv_.wait(lock, [this](){return frame_buffer_.size() < frame_capacity_;}); // block until the buffer is not full
                frame_buffer_.push_back(avframe);
                lock.unlock();
                //logger->info("frame buffer number {}", frame_buffer_.size());
            }
            av_packet_unref(packet);
        }

        avcodec_close(codec_ctx);
        avformat_close_input(&fmt_ctx);
    }

    /**
     * @brief Get the frame object from the frame_buffer
     * 
     * @return std::shared_ptr<AVFrame> 
     */
    std::shared_ptr<AVFrame> ffmpeg_producer::get_frame()
    {
        std::shared_ptr<AVFrame> frame;
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            if(frame_buffer_.size() > 0)
            {
                frame = frame_buffer_[0];
                frame_buffer_.pop_front();
                frame_cv_.notify_all();
            }            
        }

        return frame;
    }


} // namespace seeder::fffmpeg