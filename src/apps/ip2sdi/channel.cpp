/**
 * @file channel.h
 * @author 
 * @brief sdi channel, capture st2110 udp packets, assemble into a frame, 
 * send the frame to the sdi card (e.g. decklink) 
 * @version 
 * @date 2022-05-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <boost/thread/thread.hpp>

#include "channel.h"
#include "core/frame.h"
#include "util/timer.h"
#include "util/logger.h"
#include "st2110/d10/network.h"

using namespace seeder::util;
namespace seeder
{
    channel::channel(channel_config& config)
    :config_(config),
    ////decklink_consumer_(std::make_unique<decklink::decklink_consumer>(config.device_id, config.format_desc)),
    ////rtp_producer_(std::make_unique<rtp::rtp_st2110_producer>(config.format_desc)),
    udp_receiver_(std::make_unique<net::udp_receiver>(config.multicast_ip))
    {
        // bind nic 
        udp_receiver_->bind_ip(config.bind_ip, config.bind_port);
    }

    channel::~channel()
    {
        stop();

        if(decklink_consumer_)
            decklink_consumer_.reset();
        
        if(rtp_producer_)
            rtp_producer_.reset();

        if(udp_receiver_)
            udp_receiver_.reset();

        if(sdl_consumer_)
            sdl_consumer_.reset();
    }
        
    /**
     * @brief start sdi channel, capture sdi(decklink) input frame, 
     * encode to st2110 packets, send to net by udp
     * 
     */
    void channel::start()
    {
        abort_ = false;

        // start sdi producer in separate thread
        //start_decklink();

        // start rtp consume in separate thread
        //start_rtp();

        // start sdl consume in separate thread
        //start_sdl();
        
        // start udp consume in separate thread
        //start_udp();

        // do main loop, capture sdi(decklink) input frame, encode to st2110 packets
       // run();

        logger->info("Channel {} has already been started", config_.device_id);
    }

    /**
     * @brief stop the sdi channel
     * 
     */
    void channel::stop()
    {
        abort_ = true;

        // wait for threads to complete
        if(decklink_consumer_) decklink_consumer_->stop();
        if(decklink_thread_ && decklink_thread_->joinable())
        {
            decklink_thread_->join();
            decklink_thread_.reset();
        }

        if(rtp_thread_ && rtp_thread_->joinable())
        {
            rtp_thread_->join();
            rtp_thread_.reset();
        }

        if(udp_thread_ && udp_thread_->joinable())
        {
            udp_thread_->join();
            udp_thread_.reset();
        }

        if(sdl_thread_ && sdl_thread_->joinable())
        {
            sdl_thread_->join();
            sdl_thread_.reset();
        }
        if(sdl_consumer_)
        {
            sdl_consumer_.reset();
        }

        if(channel_thread_ && channel_thread_->joinable())
        {
            channel_thread_->join();
            channel_thread_.reset();
        }

        logger->info("Channel {} has already been stopped", config_.device_id);
    }

    // start sdi producer in separate thread
    void channel::start_decklink()
    {
        decklink_thread_ = std::make_unique<std::thread>([&](){
            try
            {
                this->decklink_consumer_->start();
            }
            catch(const std::exception& e)
            {
                logger->error("channel {} decklink consumer error {}", config_.device_id, e.what());
            }
        });
    }

    // start rtp consume in separate thread
    void channel::start_rtp()
    {
        rtp_thread_ = std::make_unique<std::thread>([&](){
            while(!abort_)
            {
                try
                {
                   this->rtp_producer_->run();
                }
                catch(const std::exception& e)
                {
                    logger->error("channel {} rtp error {}", config_.device_id, e.what());
                }
            }
        });
    }

    // start udp receive in separate thread
    void channel::start_udp()
    {
        udp_thread_ = std::make_unique<std::thread>([&](){
            while(!abort_)
            {
                try
                {
                   std::shared_ptr<rtp::packet> packet = std::make_shared<rtp::packet>(d10::MAX_PACKET_SIZE);
                   auto len = udp_receiver_->receive(packet->get_data_ptr(), d10::MAX_PACKET_SIZE);
                   packet->reset_size(len); // set the packet size by the true receive size

                   // push the packet to the rtp producer buffer
                   //rtp_producer_->send_packet(packet);
                }
                catch(const std::exception& e)
                {
                    logger->error("channel {} upd receiver error {}", config_.device_id, e.what());
                }
            }
        });        
    }

    // start sdl consume in separate thread
    void channel::start_sdl()
    {
        if(!config_.display_screen)
            return;
        
        sdl_consumer_ = std::make_unique<sdl::sdl_consumer>();
        sdl_thread_ = std::make_unique<std::thread>([&](){
            while(!abort_)
            {
                try
                {
                    sdl_consumer_->run();
                }
                catch(const std::exception& e)
                {
                    logger->error("channel {} sdl error {}", config_.device_id, e.what());
                }
            }
        });
    }

    // do main loop, capture sdi(decklink) input frame, encode to st2110 packets
    void channel::run()
    {
        channel_thread_ = std::make_unique<std::thread>([&](){
            auto start_time = util::timer::now();
            while(!abort_)
            {
                try
                {
                    // receive rtp frame
                    auto frame = rtp_producer_->get_frame();
                    if(frame)
                    {
                        // push to decklink output
                        decklink_consumer_->send_frame(frame);
                        
                        // if(sdl_consumer_)
                        // {
                        //     // push to sdl screen display
                        //     sdl_consumer_->send_frame(frame);
                        // }
                    }

                    boost::this_thread::sleep_for(boost::chrono::milliseconds(int(1000/config_.format_desc.fps)));  // 25 frames per second
                }
                catch(const std::exception& e)
                {
                    logger->error("channel {} error {}", config_.device_id, e.what());
                }
            }
        });
    }

} // namespace seeder