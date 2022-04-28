/**
 * @file channel.h
 * @author your name (you@domain.com)
 * @brief sdi channel, capture sdi(decklink) input frame, 
 * encode to st2110 packets, send to net by udp
 * @version 0.1
 * @date 2022-04-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "decklink/producer/decklink_producer.h"
#include "rtp/consumer/rtp_st2110_consumer.h"
#include "net/udp_client.h"
#include "sdl/consumer/sdl_consumer.h"

namespace seeder
{
    class channel
    {
      public:
        channel();
        ~channel();

        /**
         * @brief start sdi channel, capture sdi(decklink) input frame, 
         * encode to st2110 packets, send to net by udp
         * 
         */
        void start();
      
      private:
        decklink::decklink_producer decklink_producer_;
        rtp::rtp_st2110_consumer rtp_consumer_;
        net::udp_client udp_client_;
        sdl::sdl_consumer sdl_consumer_;

    };
} // namespace seeder