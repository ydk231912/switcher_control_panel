/**
 * rtp util
 *
 */
#pragma once

#include <stdint.h>

namespace seeder::rtp {

/**
 * @brief Get the video rtp timestamp 
 * 
 * @return uint32_t 
 */
uint32_t get_video_timestamp(int32_t leap);

} // namespace seeder::rtp
