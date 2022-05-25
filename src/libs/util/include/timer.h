/**
 * @file timer.h
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
    class timer
    {
        std::int_least64_t start_time_;

      public:
        /**
         * @brief get system current precise time
         * 
         * @return int64_t //milliseconds since 1970.1.1 0:0:0
         */
        static int64_t now()
        {
            std::chrono::milliseconds ms = std::chrono::duration_cast <std::chrono::milliseconds> (
                    std::chrono::system_clock::now().time_since_epoch()); 

            return ms.count();
        }

        timer() { start_time_ = now(); }

        void restart() { start_time_ = now(); }

        double elapsed() const { return static_cast<double>(now() - start_time_);}

    };
}// namespace seeder::util