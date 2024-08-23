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