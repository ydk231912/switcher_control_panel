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

#include "ffmpeg/input/ffmpeg_input.h"
#include "rtp/input/rtp_st2110_input.h"
#include "decklink/output/decklink_output.h"
#include "rtp/rtp_context.h"
#include "sdl/output/sdl_output.h"
#include "config.h"

namespace seeder
{
    class channel
    {
      public:
        channel(channel_config& config);
        ~channel();

        /**
         * @brief start sdi channel, capture rtp input frame, 
         * decode st2110 packets, send to sdi(decklink) card
         * 
         */
        void start();

        /**
         * @brief stop the sdi channel
         * 
         */
        void stop();
      
      private:
        void run();
        
      
      private:
        bool abort_ = false;
        channel_config config_;
        std::unique_ptr<decklink::decklink_output> decklink_output_ = nullptr;
        std::unique_ptr<ffmpeg::ffmpeg_input> ffmpeg_input_ = nullptr; // for test
        std::unique_ptr<rtp::rtp_st2110_input> rtp_input_ = nullptr;
        rtp::rtp_context rtp_context_;
        std::unique_ptr<sdl::sdl_output> sdl_output_ = nullptr;
        
        //thread
        std::unique_ptr<std::thread> channel_thread_ = nullptr;

    };
} // namespace seeder