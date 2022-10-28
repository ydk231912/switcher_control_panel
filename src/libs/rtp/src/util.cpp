/**
 * rtp util
 *
 */
#include "util.h"

#include "core/util/date.h"
#include "rtp/rtp_constant.h"


namespace seeder::rtp {

/**
 * @brief Get the video rtp timestamp 
 * 
 * @return uint32_t 
 */
uint32_t get_video_timestamp(int32_t leap)
{
    auto time_now_ = core::date::now() + leap * 1000000; //microsecond. 37s: leap second, since 1970-01-01 to 2022-01-01
    auto video_time = static_cast<uint64_t>(round(time_now_ * 0.09)); //RTP_VIDEO_RATE / 1000000));
    uint32_t timestamp = static_cast<uint32_t>(video_time % RTP_WRAP_AROUND);
    return timestamp;
}
    
}
