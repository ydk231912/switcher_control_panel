/**
 * @file rtp_st2110_consumer.h
 * @author 
 * @brief recieve sdi frame, encodec to st2110 packet
 * @version 
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <memory>
#include <mutex>
#include <deque>
#include <condition_variable>

#include "core/frame.h"
#include "rtp/packet.h"
#include "net/udp_sender.h"

namespace seeder::rtp
{
    class rtp_st2110_consumer
    {
      public:
        rtp_st2110_consumer();
        ~rtp_st2110_consumer();

        /**
         * @brief start consumer:read frame from the frame buffer, 
         * encode to st2110 udp packet, put the packet into packet buffer
         * 
         */
        void run();

        /**
         * @brief send frame to the rtp st2110 consumer 
         * 
         */
        void send_frame(core::frame);

        /**
         * @brief Get the frame from the frame buffer
         * 
         * @return core::frame 
         */
        core::frame get_frame();

        /**
         * @brief Get the st2110 packet 
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


      private:
        uint16_t sequence_number_;
        std::mutex frame_mutex_, packet_mutex_;
        std::deque<core::frame> frame_buffer_;
        std::deque<std::shared_ptr<rtp::packet>> packet_buffer_;
        const std::size_t frame_capacity_ = 10;
        const std::size_t packet_capacity_ = 30000; // 30k * 1.4k = 42M 
        const std::size_t chunks_per_frame_ = 10;
        uint16_t height_;
        std::condition_variable frame_cv_;
        std::condition_variable packet_cv_;
        
        const std::size_t batch_send_size_ = 600;
        bool is_frame_complete_ = false;

        std::unique_ptr<net::udp_sender> udp_sender_ = nullptr;



    };
}