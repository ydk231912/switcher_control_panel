/**
 * @file timer.h
 * @author 
 * @brief get steady clock time point
 * @version 
 * @date 2022-04-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

namespace seeder::core
{
    class timer
    {
        std::int_least64_t start_time_;

      public:
        /**
         * @brief get system current precise time
         * 
         * @return int64_t //nanoseconds since 1970.1.1 0:0:0 utc
         */
        static int64_t now()
        {
            std::chrono::nanoseconds ns = std::chrono::duration_cast <std::chrono::nanoseconds> (
                    std::chrono::steady_clock::now().time_since_epoch()); 

            return ns.count(); 
        }

        timer() { start_time_ = now(); }

        void restart() { start_time_ = now(); }

        double elapsed() const { return static_cast<double>(now() - start_time_);}

        double elapsed_milliseconds() const { return static_cast<double>((now() - start_time_) / 1000000);}

        double elapsed_microseconds() const { return static_cast<double>((now() - start_time_) / 1000);}

    };
}// namespace seeder::core