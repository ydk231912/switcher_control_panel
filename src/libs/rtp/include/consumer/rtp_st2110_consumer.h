/**
 * @file rtp_st2110_consumer.h
 * @author your name (you@domain.com)
 * @brief recieve sdi frame, encodec to st2110 packet
 * @version 0.1
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <memory>
#include <mutex>
#include <deque>

#include "util/frame.h"
#include "rtp/packet.h"

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
        void start();

        /**
         * @brief send frame to the rtp st2110 consumer 
         * 
         */
        void send_frame(util::frame);

        /**
         * @brief Get the st2110 packet 
         * 
         * @return std::shared_ptr<packet> 
         */
        std::shared_ptr<packet> get_packet();


      private:
        uint16_t sequence_number_;
        std::mutex frame_mutex_, packet_mutex_;
        std::deque<util::frame> frame_buffer_;
        std::deque<std::shared_ptr<rtp::packet>> packet_buffer_;
        const std::size_t frame_capacity_ = 10;
        const std::size_t packet_capacity_ = 300000; // 300k * 1.4k = 420M 


    };
}