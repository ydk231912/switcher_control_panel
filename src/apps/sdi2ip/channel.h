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

#include "decklink/input/decklink_input.h"
#include "ffmpeg/input/ffmpeg_input.h"
#include "rtp/output/rtp_st2110_output.h"
#include "rtp/output/rtp_st2110_output2.h"
#include "rtp/output/rtp_st2110_output_bk.h"
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
        void run();
        
      private:
        bool abort_ = false;
        channel_config config_;
        std::unique_ptr<decklink::decklink_input> decklink_input_ = nullptr;
        std::unique_ptr<ffmpeg::ffmpeg_input> ffmpeg_input_ = nullptr; // for test
        std::unique_ptr<rtp::rtp_st2110_output> rtp_output_ = nullptr;
        std::unique_ptr<rtp::rtp_st2110_output2> rtp_output2_ = nullptr;
        std::unique_ptr<rtp::rtp_st2110_output_bk> rtp_output_bk_ = nullptr;
        rtp::rtp_context rtp_context_;
        std::unique_ptr<sdl::sdl_output> sdl_output_ = nullptr;
        
        //thread
        std::unique_ptr<std::thread> channel_thread_ = nullptr;

    };
} // namespace seeder