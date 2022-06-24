/**
 * @file date.h
 * @author 
 * @brief get system current precise time
 * @version 
 * @date 2022-04-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

namespace seeder::util
{
    class date
    {
        std::int_least64_t start_time_;

      public:
        /**
         * @brief get system current precise time
         * 
         * @return int64_t //microsecond since 1970.1.1 0:0:0 utc
         */
        static int64_t now()
        {
            std::chrono::microseconds ns = std::chrono::duration_cast <std::chrono::microseconds> (
                    std::chrono::system_clock::now().time_since_epoch()); 

            return ns.count(); 
        }

        date() { start_time_ = now(); }

        void restart() { start_time_ = now(); }

        double elapsed() const { return static_cast<double>(now() - start_time_);}

    };
}// namespace seeder::util