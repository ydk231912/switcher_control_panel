/**
 * @file rtp_st2110_producer.h
 * @author 
 * @brief recieve st2110 udp packets, decode to video frame
 * @version 
 * @date 2022-05-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <memory>
#include <mutex>
#include <deque>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "core/frame.h"
#include "core/video_format.h"
#include "rtp/packet.h"

namespace seeder::rtp
{
    class rtp_st2110_producer
    {
      public:
        rtp_st2110_producer(core::video_format_desc format_desc);
        ~rtp_st2110_producer();

        /**
         * @brief start producer:read packet from packet buffer, 
         * decode to frame, put the frame into frame buffer
         * 
         */
        void run();

        /**
         * @brief send the frame to the rtp frame buffer 
         * 
         */
        void send_frame(std::shared_ptr<AVFrame>);

        /**
         * @brief Get the frame from the frame buffer
         * 
         * @return core::frame 
         */
        std::shared_ptr<AVFrame> get_frame();

        /**
         * @brief Get the st2110 packet from the packet buffer
         * 
         * @return std::shared_ptr<packet> 
         */
        std::shared_ptr<packet> get_packet();

        /**
         * @brief Get the packet batch
         * 
         * @return std::vector<std::shared_ptr<packet>> 
         */
        std::vector<std::shared_ptr<packet>> get_packet_batch();

        /**
         * @brief send the packet to the packet buffer
         * 
         */
        void send_packet(std::shared_ptr<packet>);


      private:
        bool need_new_frame_ = true;
        uint32_t last_frame_timestamp_ = 0;
        std::shared_ptr<AVFrame> current_frame_ = nullptr;
        core::video_format_desc format_desc_;
        std::mutex frame_mutex_, packet_mutex_;
        std::deque<std::shared_ptr<AVFrame>> frame_buffer_;
        std::deque<std::shared_ptr<rtp::packet>> packet_buffer_;
        const std::size_t frame_capacity_ = 10;
        const std::size_t packet_capacity_ = 30000; // 30k * 1.4k = 42M 
        const std::size_t chunks_per_frame_ = 10;
        uint16_t height_;

        const float samples_per_pixel_ = 2; //YCbCr_4_2_2:2; YCbCr_4_4_4: 3; RGB_4_4_4: 3;YCbCr_4_2_0: 1.5
        const int color_depth_ = 10;
    };
}