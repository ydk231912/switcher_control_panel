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

namespace seeder::core
{
    /**
     * @brief output stream interface
     * all output devices need to implement this interface,
     * output devices include: decklink sdi output, video file capture, rtp output stream etc.
     * 
     */
    class output
    {
      public:
        /**
         * @brief start output stream handle
         * 
         */
        virtual void start() = 0;
        
        /**
         * @brief stop output stream handle
         * 
         */
        virtual void stop() = 0;

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
        virtual void display_video_frame(uint8_t* vframe) = 0;

        /**
         * @brief push a audio frame into this output stream
         * 
         */
        virtual void display_audio_frame(uint8_t* aframe) = 0;

        // frame buffer pointer
        uint8_t* vframe_buffer;


    };
} 