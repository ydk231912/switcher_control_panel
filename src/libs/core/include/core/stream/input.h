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

#include <atomic>
#include <string>
#include <memory>

#include <boost/core/noncopyable.hpp>

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

      protected:
        virtual void do_stop();

      private:
        std::string source_id;
        std::atomic<int> status = {running_status::INIT};
    };
} 