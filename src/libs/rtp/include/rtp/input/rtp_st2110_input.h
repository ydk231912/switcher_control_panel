/**
 * @file rtp_st2110_input.h
 * @author 
 * @brief recieve st2110 udp packets, decode to video frame
 * @version 
 * @date 2022-07-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <memory>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <thread>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "core/video_format.h"
#include "rtp/packet.h"
#include "core/frame/frame.h"
#include "core/stream/input.h"
#include "rtp/rtp_context.h"
#include "net/udp_receiver.h"

namespace seeder::rtp
{
    class rtp_st2110_input : public core::input
    {
      public:
        explicit rtp_st2110_input(const std::string &source_id, rtp_context context);
        ~rtp_st2110_input();

        /**
         * @brief start producer:read packet from packet buffer, 
         * decode to frame, put the frame into frame buffer
         * 
         */
        void start();

        void stop();

        void decode();

        /**
         * @brief set the frame to the rtp frame buffer 
         * 
         */
        void set_frame(std::shared_ptr<core::frame>);

        /**
         * @brief Get the frame from the frame buffer
         * 
         * @return core::frame 
         */
        std::shared_ptr<core::frame> get_frame();

        /**
         * @brief Get the st2110 packet from the packet buffer
         * 
         * @return std::shared_ptr<packet> 
         */
        std::shared_ptr<packet> get_packet();

        /**
         * @brief Get the packet batch
         * 
         * @return std::deque<std::shared_ptr<rtp::packet>>
         */
        std::deque<std::shared_ptr<rtp::packet>> get_packet_batch();

        /**
         * @brief add packet to input stream buffer
         * 
         * @param packet 
         */
        void set_packet(std::shared_ptr<packet>);

        /**
         * @brief receive packet by udp multicast
         * 
         */
        void receive_packet();


      private:
        bool abort = false;
        uint32_t last_frame_timestamp_ = 0;
        std::shared_ptr<core::frame> current_frame_ = nullptr;
        uint8_t* buffer_;
        core::video_format_desc format_desc_;
        rtp_context context_;
        std::mutex frame_mutex_, packet_mutex_;
        std::deque<std::shared_ptr<core::frame>> frame_buffer_;
        std::deque<std::shared_ptr<rtp::packet>> packet_buffer_;
        const std::size_t frame_capacity_ = 10;
        const std::size_t packet_capacity_ = 30000; // 30k * 1.4k = 42M 
        std::condition_variable packet_cv_;
        net::udp_receiver receiver_;

        std::unique_ptr<std::thread> st2110_thread_;
        std::unique_ptr<std::thread> udp_thread_;        

        const float samples_per_pixel_ = 2; //YCbCr_4_2_2:2; YCbCr_4_4_4: 3; RGB_4_4_4: 3;YCbCr_4_2_0: 1.5
        const int color_depth_ = 10;
        int frame_size_ = 0;
    };
}