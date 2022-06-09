/**
 * @file channel.h
 * @author 
 * @brief sdi channel, capture sdi(decklink) input frame, 
 * encode to st2110 packets, send to net by udp
 * @version 
 * @date 2022-04-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <thread>

#include "decklink/producer/decklink_producer.h"
#include "rtp/consumer/rtp_st2110_consumer.h"
#include "net/udp_sender.h"
#include "sdl/consumer/sdl_consumer.h"
#include "config.h"
#include "ffmpeg/producer/ffmpeg_producer.h"

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
        void start_ffmpeg();
        void start_rtp();
        void start_udp();
        void start_sdl();
        void run();
      
      private:
        channel_config config_;
        std::unique_ptr<decklink::decklink_producer> decklink_producer_ = nullptr;
        std::unique_ptr<rtp::rtp_st2110_consumer> rtp_consumer_ = nullptr;
        std::unique_ptr<net::udp_sender> udp_sender_ = nullptr;
        std::unique_ptr<sdl::sdl_consumer> sdl_consumer_ = nullptr;
        std::unique_ptr<ffmpeg::ffmpeg_producer> ffmpeg_producer_ = nullptr; // for test

        //thread
        std::unique_ptr<std::thread> decklink_thread_ = nullptr;
        std::unique_ptr<std::thread> rtp_thread_ = nullptr;
        std::unique_ptr<std::thread> udp_thread_ = nullptr;
        std::unique_ptr<std::thread> sdl_thread_ = nullptr;
        std::unique_ptr<std::thread> channel_thread_ = nullptr;
        std::unique_ptr<std::thread> ffmpeg_thread_ = nullptr; // for test
        bool abort_ = false;

    };
} // namespace seeder