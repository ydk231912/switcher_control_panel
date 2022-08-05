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
#include <unistd.h>

#include "channel.h"
#include "core/frame/frame.h"
#include "core/util/timer.h"
#include "core/util/date.h"
#include "core/util/logger.h"

using namespace seeder::core;
namespace seeder
{
    channel::channel(channel_config& config)
    :config_(config)
    ,rtp_context_(config.format_desc, config.multicast_ip, config.multicast_port)
    ,decklink_output_(std::make_unique<decklink::decklink_output>(config.device_id, config.format_desc))
    ,rtp_input_(std::make_unique<rtp::rtp_st2110_input>(rtp_context_))
    ,ffmpeg_input_(std::make_unique<ffmpeg::ffmpeg_input>("/home/seeder/c.MXF")) // for test
    {
        if(config.display_screen)
            sdl_output_ = std::make_unique<sdl::sdl_output>(config_.format_desc);
    }

    channel::~channel()
    {
        stop();

        if(decklink_output_)
            decklink_output_.reset();

        if(ffmpeg_input_)
            ffmpeg_input_.reset();
        
        if(rtp_input_)
            rtp_input_.reset();

        if(sdl_output_)
            sdl_output_.reset();
    }
        
    /**
     * @brief start sdi channel, capture rtp input frame, 
     * decode st2110 packets, send to sdi(decklink) card
     * 
     */
    void channel::start()
    {
        abort_ = false;

        rtp_input_->start();

        if(ffmpeg_input_)
            ffmpeg_input_->start();

        decklink_output_->start();

        if(sdl_output_)
            sdl_output_->start();
        
        // do main loop, capture rtp input stream, decode st2110 packets to frame, send to sdi card 
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

        if(rtp_input_)
            rtp_input_->stop();

        if(ffmpeg_input_)
            ffmpeg_input_->stop();
        
        if(decklink_output_)
            decklink_output_->stop();

        if(sdl_output_)
            sdl_output_->stop();

        logger->info("Channel {} has already been stopped", config_.device_id);
    }

    // do main loop, capture sdi(decklink) input frame, encode to st2110 packets
    void channel::run()
    {
        channel_thread_ = std::make_unique<std::thread>([&](){
            // cpu_set_t mask;
            // CPU_ZERO(&mask);
            // CPU_SET(5, &mask);
            // if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
            // {
            //     logger->error("bind udp thread to cpu failed");
            // }

            while(!abort_)
            {
                try
                {
                    // capture sdi frame
                    auto frame = rtp_input_->get_frame();
                    //auto frame = ffmpeg_input_->get_frame();
                    if(frame)
                    {
                        // push to rtp
                        decklink_output_->set_frame(frame);
                        
                        if(sdl_output_)
                            // push to sdl screen display
                            sdl_output_->set_frame(frame);
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
