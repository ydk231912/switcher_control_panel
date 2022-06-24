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
#include <unistd.h>

#include "channel.h"
#include "core/frame.h"
#include "util/timer.h"
#include "util/date.h"
#include "util/logger.h"

using namespace seeder::util;
using namespace seeder::core;
namespace seeder
{
    channel::channel(channel_config& config)
    :config_(config),
    rtp_consumer_(std::make_unique<rtp::rtp_st2110_consumer>()),
    udp_sender_(std::make_unique<net::udp_sender>())
    {
        // bind nic 
        udp_sender_->bind_ip(config.bind_ip, config.bind_port);

        // packet drain interval/duration  nanosecond
        packet_drain_interval_ = static_cast<int64_t>(((1000000000/config_.format_desc.fps) / 3848) / 1.1);// (t_frame / n_pakcets) * (1 / B) defined by st2110-21
    }

    channel::~channel()
    {
        stop();

        if(decklink_producer_)
            decklink_producer_.reset();

        if(ffmpeg_producer_)
            ffmpeg_producer_.reset();
        
        if(rtp_consumer_)
            rtp_consumer_.reset();

        if(udp_sender_)
            udp_sender_.reset();

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
        start_ffmpeg();

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
        if(decklink_producer_)
        {
            decklink_producer_->stop();
            decklink_producer_.reset();
        }
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
                decklink_producer_ = std::make_unique<decklink::decklink_producer>(config_.device_id, config_.format_desc);
                this->decklink_producer_->start();
            }
            catch(const std::exception& e)
            {
                logger->error("channel {} decklink producer error {}", config_.device_id, e.what());
            }
        });
    }

    /**
     * @brief for test
     * start ffmpeg producer in separate thread
     */
    void channel::start_ffmpeg()
    {
        ffmpeg_thread_ = std::make_unique<std::thread>([&](){
            try
            {
                ffmpeg_producer_ = std::make_unique<ffmpeg::ffmpeg_producer>();
                while(!abort_) // loop playback
                {
                    ffmpeg_producer_->run("/home/seeder/c.MXF");
                }
            }
            catch(const std::exception& e)
            {
                logger->error("channel {} ffmpeg producer error {}", config_.device_id, e.what());
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
        util::logger->info("start udp sender test92 ");

        packet_drained_number_ = 0;

        udp_thread_ = std::make_unique<std::thread>([&](){
            // bind thread to one fixed cpu core
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(1, &mask);
            if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
            {
                util::logger->error("bind udp thread to cpu failed");
            }

            while(!abort_)
            {
                try
                {
                    //send_to single packet 
                    auto packet = rtp_consumer_->get_packet();
                    if(packet)
                    {
                        if(packet_drained_number_ == 0)
                        {
                            start_time_ = timer::now();
                        }
                        auto elapse = timer::now() - start_time_;
                        auto drain_elapse = packet_drained_number_ * packet_drain_interval_;
                        while(drain_elapse > elapse) // wait until time point arrive
                        {
                            elapse = timer::now() - start_time_;
                        }
                        packet_drained_number_++;
                        udp_sender_->send_to(packet->get_data_ptr(), packet->get_data_size(), config_.multicast_ip, config_.multicast_port);
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
            auto time_stamp = util::date::now() + 37000000; //37s: leap second, since 1970-01-01 to 2022-01-01
            auto base = static_cast<uint32_t>(pow(2, 32));

            while(!abort_)
            {
                try
                {
                    // capture sdi frame
                    //auto avframe = decklink_producer_->get_frame();
                    auto avframe = ffmpeg_producer_->get_frame();
                    if(avframe)
                    {
                        core::frame frame;
                        frame.video = avframe;
                        auto duration = (avframe->pts + 1) / config_.format_desc.fps * 1000000; //the milliseconds from play start time to now
                        auto time = static_cast<uint64_t>((time_stamp + duration) * 90 / 1000);
                        frame.timestamp = static_cast<uint32_t>(time % base); //timestamp = seconds * 90000 mod 2^32
                        //std::cout << frame.timestamp << std::endl;

                        // push to rtp
                        rtp_consumer_->send_frame(frame);
                        
                        if(sdl_consumer_)
                        {
                            // push to sdl screen display
                            sdl_consumer_->send_frame(frame.video);
                        }
                    }

                    //boost::this_thread::sleep_for(boost::chrono::milliseconds(int(1000/config_.format_desc.fps)));  // 25 frames per second
                }
                catch(const std::exception& e)
                {
                    logger->error("channel {} error {}", config_.device_id, e.what());
                }
            }
        });
    }

} // namespace seeder

////////////////////////////////////////////////////////////////// backup
    // udp
    // void channel::start_udp()
    // {
    //     util::logger->info("start udp sender test92 ");

    //     packet_drained_number_ = 0;

    //     udp_thread_ = std::make_unique<std::thread>([&](){
    //         while(!abort_)
    //         {
    //             try
    //             {
    //                 // sendmmsg_to multi-packets 
    //                 // auto packets = rtp_consumer_->get_packet_batch();
    //                 // if(packets.size() > 0)
    //                 // {
    //                 //     //std::cout << packets.size() << std::endl;
    //                 //     std::vector<net::send_data> vdata;
    //                 //     vdata.reserve(packets.size());
    //                 //     for(auto p : packets)
    //                 //     {
    //                 //         net::send_data sd;
    //                 //         sd.data = p->get_data_ptr();
    //                 //         sd.length = p->get_data_size();
    //                 //         vdata.push_back(sd);
    //                 //     }
    //                 //     udp_sender_->sendmmsg_to(vdata, config_.multicast_ip, config_.multicast_port);
    //                 // }

    //                 //send_to single packet 
    //                 auto packet = rtp_consumer_->get_packet();
    //                 if(packet)
    //                 {
    //                     if(packet_drained_number_ == 0)
    //                     {
    //                         start_time_ = timer::now();
    //                     }
    //                     auto elapse = timer::now() - start_time_;
    //                     auto drain_elapse = packet_drained_number_ * packet_drain_interval_;
    //                     while(drain_elapse > elapse) // wait until time point arrive
    //                     {
    //                         elapse = timer::now() - start_time_;
    //                     }
    //                     packet_drained_number_++;
    //                     udp_sender_->send_to(packet->get_data_ptr(), packet->get_data_size(), config_.multicast_ip, config_.multicast_port);
    //                 }

    //             }
    //             catch(const std::exception& e)
    //             {
    //                 logger->error("channel {} upd error {}", config_.device_id, e.what());
    //             }
    //         }
    //     });
    // }