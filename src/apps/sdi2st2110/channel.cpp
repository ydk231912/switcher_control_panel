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

#include <boost/thread/thread.hpp>

#include "channel.h"
#include "util/frame.h"
#include "util/timer.h"
#include "util/logger.h"

using namespace seeder::util;
namespace seeder
{
    channel::channel(channel_config& config)
    :config_(config),
    rtp_consumer_(std::make_shared<rtp::rtp_st2110_consumer>()),
    udp_client_(std::make_shared<net::udp_client>())
    {
        // bing nic 
        udp_client_->bind_ip(config.bind_ip, config.bind_port);
    }

    channel::~channel()
    {
        stop();

        if(decklink_producer_)
            decklink_producer_.reset();
        
        if(rtp_consumer_)
            rtp_consumer_.reset();

        if(udp_client_)
            udp_client_.reset();

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
        start_decklink();

        // start rtp consume in separate thread
        start_rtp();

        // start sdl consume in separate thread
        start_sdl();
        
        // start udp consume in separate thread
        start_udp();

        // do main loop, capture sdi(decklink) input frame, encode to st2110 packets
        run();

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
        if(decklink_thread_ && decklink_thread_->joinable())
        {
            decklink_thread_->join();
            decklink_thread_.reset();
        }
        if(decklink_producer_)
        {
            decklink_producer_->stop();
            decklink_producer_.reset();
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
        decklink_thread_ = std::make_shared<std::thread>([&](){
            try
            {
                decklink_producer_ = std::make_shared<decklink::decklink_producer>(config_.device_id, config_.format_desc);
                this->decklink_producer_->start();
            }
            catch(const std::exception& e)
            {
                logger->error("channel {} decklink producer error {}", config_.device_id, e.what());
            }
        });
    }

    // start rtp consume in separate thread
    void channel::start_rtp()
    {
        rtp_thread_ = std::make_shared<std::thread>([&](){
            while(!abort_)
            {
                try
                {
                   this->rtp_consumer_->run();
                }
                catch(const std::exception& e)
                {
                    logger->error("channel {} rtp error {}", config_.device_id, e.what());
                }
            }
        });
    }

    // start udp consume in separate thread
    void channel::start_udp()
    {
        udp_thread_ = std::make_shared<std::thread>([&](){
            while(!abort_)
            {
                try
                {
                    auto packets = rtp_consumer_->get_packet_batch();
                    if(packets.size() > 0)
                    {
                        std::vector<net::send_data> vdata;
                        for(auto p : packets)
                        {
                            net::send_data sd;
                            sd.data = p->get_data_ptr();
                            sd.length = p->get_data_size();
                            vdata.push_back(sd);
                        }
                        udp_client_->sendmmsg_to(vdata, config_.multicast_ip, config_.multicast_port);
                    }
                    else
                    {
                        boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
                    }
                }
                catch(const std::exception& e)
                {
                    logger->error("channel {} upd error {}", config_.device_id, e.what());
                }
            }
        });        
    }

    // start sdl consume in separate thread
    void channel::start_sdl()
    {
        if(!config_.display_screen)
            return;
        
        sdl_consumer_ = std::make_shared<sdl::sdl_consumer>();
        sdl_thread_ = std::make_shared<std::thread>([&](){
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
        channel_thread_ = std::make_shared<std::thread>([&](){
            auto start_time = util::timer::now();
            while(!abort_)
            {
                try
                {
                    // capture sdi frame
                    auto avframe = decklink_producer_->get_frame();
                    if(avframe)
                    {
                        util::frame frame;
                        frame.video = std::move(avframe);
                        auto duration = int(config_.format_desc.fps * 1000 * avframe->pts); //the milliseconds from play start time to now
                        frame.timestamp = int(((start_time + duration) * 90) % int(pow(2, 32))); //timestamp = seconds * 90000 mod 2^32
                        
                        // push to rtp
                        rtp_consumer_->send_frame(frame);
                        
                        if(sdl_consumer_)
                        {
                            // push to sdl screen display
                            sdl_consumer_->send_frame(frame.video);
                        }
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