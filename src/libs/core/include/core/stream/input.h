/**
 * @file input.h
 * @author 
 * @brief input stream interface
 * @version 1.0
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "core/frame/frame.h"
#include "core/util/buffer.h"
#include <boost/core/noncopyable.hpp>
#include <atomic>

namespace seeder::core
{
    /**
     * @brief input stream interface
     * all input devices need to implement this interface,
     * input devices include: decklink sdi input, video file, rtp input stream etc.
     * 
     */
    class input: boost::noncopyable
    {
      public:
        /**
         * @brief start input stream handle
         * 
         */
        virtual void start();
        
        /**
         * @brief stop input stream handle
         * 
         */
        virtual void stop();

        /**
         * @brief push a frame into this input stream
         * 
         */
        virtual void set_frame(std::shared_ptr<frame> frm) = 0;

        /**
         * @brief push a video frame into this input stream
         * 
         */
        virtual void set_video_frame(std::shared_ptr<AVFrame> vframe) = 0;

        /**
         * @brief push a audio frame into this input stream
         * 
         */
        virtual void set_audio_frame(std::shared_ptr<AVFrame> aframe) = 0;

        /**
         * @brief Get a frame from this input stream
         * 
         */
        virtual std::shared_ptr<frame> get_frame() = 0;

        /**
         * @brief Get a video frame from this input stream
         * 
         */
        virtual std::shared_ptr<AVFrame> get_video_frame() = 0;

        /**
         * @brief Get a audio frame from this input stream
         * 
         */
        virtual std::shared_ptr<AVFrame> get_audio_frame() = 0;

        /**
         * @brief push a audio slice frame into this input stream
         * 
         */
        virtual void set_audio_frame_slice(std::shared_ptr<buffer> asframe) = 0;

        /**
         * @brief Get a audio slice frame from this input stream
         * 
         */
        virtual std::shared_ptr<buffer> get_audio_frame_slice() = 0;

        explicit input(const std::string &source_id);

        virtual ~input();

        const std::string & get_source_id() const;

        virtual bool is_started() const;

        virtual bool is_stopped() const;

        enum running_status {
          INIT = 0,
          STARTED = 1,
          STOPPED = 2
        };

      private:
        std::string source_id;
        std::atomic<int> status = {running_status::INIT};
    };
} 