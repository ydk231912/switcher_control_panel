/**
 * @file output.h
 * @author 
 * @brief output stream interface
 * @version 1.0
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "core/frame/frame.h"
#include <boost/core/noncopyable.hpp>
#include <atomic>
#include <string>

namespace seeder::core
{
    /**
     * @brief output stream interface
     * all output devices need to implement this interface,
     * output devices include: decklink sdi output, video file capture, rtp output stream etc.
     * 
     */
    class output: boost::noncopyable
    {
      public:
        /**
         * @brief start output stream handle
         * 
         */
        virtual void start();
        
        /**
         * @brief stop output stream handle
         * 
         */
        virtual void stop();

        virtual void consume_st_video_frame(void* frame, uint32_t width, uint32_t height) = 0;

        virtual void consume_st_audio_frame(void* frame, size_t frame_size) = 0;

        /**
         * @brief push a frame into this output stream
         * 
         */
        virtual void set_frame(std::shared_ptr<frame> frm) = 0;

        /**
         * @brief Get a frame from this output stream
         * 
         */
        virtual std::shared_ptr<frame> get_frame() = 0;

        /**
         * @brief push a video frame into this output stream
         * 
         */
        virtual void set_video_frame(std::shared_ptr<AVFrame> vframe) = 0;

        /**
         * @brief push a audio frame into this output stream
         * 
         */
        virtual void set_audio_frame(std::shared_ptr<AVFrame> aframe) = 0;

        /**
         * @brief Get a video frame from this output stream
         * 
         */
        virtual std::shared_ptr<AVFrame> get_video_frame() = 0;

        /**
         * @brief Get a audio frame from this output stream
         * 
         */
        virtual std::shared_ptr<AVFrame> get_audio_frame() = 0;

        /**
         * @brief push a video frame into this output stream
         * 
         */
        virtual void display_video_frame() = 0;

        /**
         * @brief push a audio frame into this output stream
         * 
         */
        virtual void display_audio_frame() = 0;

        virtual void dump_stat();


        explicit output(const std::string &output_id);

        virtual ~output();

        const std::string & get_output_id() const;

        virtual bool is_started() const noexcept;

        virtual bool is_stopped() const noexcept;

        enum running_status {
          INIT = 0,
          STARTED = 1,
          STOPPED = 2
        };

      protected:
        virtual void do_stop();

      private:
        std::string output_id;
        std::atomic<int> status = {running_status::INIT};
    };
} 