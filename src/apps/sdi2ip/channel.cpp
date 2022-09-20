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
    //,decklink_input_(std::make_unique<decklink::decklink_input>(config.device_id, config.format_desc))
    ,ffmpeg_input_(std::make_unique<ffmpeg::ffmpeg_input>("/home/seeder/c.MXF")) // c.MXF for test
    {
        rtp_context_.rtp_video_type = config.rtp_video_type;
        rtp_context_.rtp_audio_type = config.rtp_audio_type;
        rtp_context_.leap_seconds = config.leap_seconds;
        
        
        //rtp_output2_ = std::make_unique<rtp::rtp_st2110_output2>(rtp_context_);
        rtp_output_ = std::make_unique<rtp::rtp_st2110_output>(rtp_context_);
        rtp_output_bk_ = std::make_unique<rtp::rtp_st2110_output_bk>(rtp_context_);

        if(config.display_screen)
            sdl_output_ = std::make_unique<sdl::sdl_output>(config_.format_desc);

    }

    channel::~channel()
    {
        if(abort_ == false)
            stop();

        if(decklink_input_)
            decklink_input_.reset();

        if(ffmpeg_input_)
            ffmpeg_input_.reset();
        
        if(rtp_output_)
            rtp_output_.reset();

        if(sdl_output_)
            sdl_output_.reset();
    }
        
    /**
     * @brief start sdi channel, capture sdi(decklink) input frame, 
     * encode to st2110 packets, send to net by udp
     * 
     */
    void channel::start()
    {
        abort_ = false;

        if(decklink_input_)
            decklink_input_->start();

        if(ffmpeg_input_)
            ffmpeg_input_->start();
        
        if(rtp_output_)
            rtp_output_->start();

        if(rtp_output_bk_)
            rtp_output_bk_->start();

        if(sdl_output_)
            sdl_output_->start();


        
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

        if(decklink_input_)
            decklink_input_->stop();

        if(ffmpeg_input_)
            ffmpeg_input_->stop();
        
        if(rtp_output_)
            rtp_output_->stop();

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
                    // auto frame = decklink_input_->get_frame();
                    auto frame = ffmpeg_input_->get_frame(); // for test
                    if(frame)
                    {
                        // push to rtp
                        //rtp_output_bk_->set_frame(frame);
                        rtp_output_->set_frame(frame);
                        
                        // push to sdl screen display
                        if(sdl_output_)
                            sdl_output_->set_frame(frame); //set_avframe(frame)
                    }
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(int(1000/config_.format_desc.fps)));  // 25 frames per second
                }
                catch(const std::exception& e)
                {
                    logger->error("channel {} error {}", config_.device_id, e.what());
                }
            }
        });

        channel_thread_->join();
    }

} // namespace seeder
