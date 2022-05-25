/**
 * @file channel.h
 * @author 
 * @brief sdi channel, receive st2110 udp packets, assemble into a frame, 
 * send the frame to the sdi card (e.g. decklink) 
 * @version 
 * @date 2022-05-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <thread>

#include "decklink/producer/decklink_producer.h"
#include "rtp/producer/rtp_st2110_producer.h"
#include "net/udp_receiver.h"
#include "sdl/consumer/sdl_consumer.h"
#include "config.h"

namespace seeder
{
    class channel
    {
      public:
        channel(channel_config& config);
        ~channel();

        /**
         * @brief start sdi channel, capture sdi(decklink) input frame, 
         * encode to st2110 packets, send to net by udp
         * 
         */
        void start();

        /**
         * @brief stop the sdi channel
         * 
         */
        void stop();
      
      private:
        void start_decklink();
        void start_rtp();
        void start_udp();
        void start_sdl();
        void run();
      
      private:
        channel_config config_;
        std::shared_ptr<decklink::decklink_producer> decklink_producer_ = nullptr;
        std::shared_ptr<rtp::rtp_st2110_producer> rtp_producer_ = nullptr;
        std::shared_ptr<net::udp_receiver> udp_receiver_ = nullptr;
        std::shared_ptr<sdl::sdl_consumer> sdl_consumer_ = nullptr;

        //thread
        std::shared_ptr<std::thread> decklink_thread_ = nullptr;
        std::shared_ptr<std::thread> rtp_thread_ = nullptr;
        std::shared_ptr<std::thread> udp_thread_ = nullptr;
        std::shared_ptr<std::thread> sdl_thread_ = nullptr;
        std::shared_ptr<std::thread> channel_thread_ = nullptr;
        bool abort_ = false;

    };
} // namespace seeder