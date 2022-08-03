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

namespace seeder::core
{
    /**
     * @brief input stream interface
     * all input devices need to implement this interface,
     * input devices include: decklink sdi input, video file, rtp input stream etc.
     * 
     */
    class input
    {
      public:
        /**
         * @brief start input stream handle
         * 
         */
        virtual void start() = 0;
        
        /**
         * @brief stop input stream handle
         * 
         */
        virtual void stop() = 0;

        /**
         * @brief push a frame into this input stream
         * 
         */
        virtual void set_frame(std::shared_ptr<frame> frm) = 0;

        /**
         * @brief Get a frame from this input stream
         * 
         */
        virtual std::shared_ptr<frame> get_frame() = 0;

    };
} 