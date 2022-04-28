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

#include <boost/thread/thread.hpp>

#include "channel.h"
#include "util/frame.h"
#include "util/timer.h"

namespace seeder
{
    channel::channel()
    :decklink_producer_(),
    rtp_consumer_(),
    udp_client_(),
    sdl_consumer_()
    {
    }

    channel::~channel(){}
        
    /**
     * @brief start sdi channel, capture sdi(decklink) input frame, 
     * encode to st2110 packets, send to net by udp
     * 
     */
    void channel::start()
    {
        // start sdi product in alone thread
        std::thread decklink_thread([&](){
            this->decklink_producer_.start();
        });

        // start rtp consume in alone thread
        std::thread rtp_thread([&](){
            this->rtp_consumer_.start();
        });

        // start sdl consume in alone thread
        std::thread sdl_thread([&](){
            this->sdl_consumer_.start();
        });

        // start udp consume in alone thread
        std::thread udp_thread([&](){
            while(true)
            {
                auto packet = rtp_consumer_.get_packet();
                if(packet)
                {
                    udp_client_.send_to(packet->get_data_ptr(), packet->get_data_size(), "239.0.0.1", 20000);
                }
                else
                {
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
                }
            }
        });

        // do main loop, capture sdi(decklink) input frame, encode to st2110 packets
        std::thread channel_thread([&](){
            auto start_time = util::timer::now();
            auto fps = 25;
            while(true)
            {
                auto avframe = decklink_producer_.get_frame();
                if(avframe)
                {
                    util::frame frame;
                    frame.video = std::move(avframe);
                    auto duration = fps * 1000 * avframe->pts; //the milliseconds from play start time to now
                    frame.timestamp = (int)(((start_time + duration) * 90) % (int)pow(2, 32)); //timestamp = seconds * 90000 mod 2^32
                    rtp_consumer_.send_frame(frame); // push to rtp
                }

                boost::this_thread::sleep_for(boost::chrono::milliseconds(1000/fps));  // 25 frames per second
            }
        });
    }
} // namespace seeder