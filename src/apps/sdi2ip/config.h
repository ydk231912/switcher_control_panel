/**
 * @file config.h
 * @author 
 * @brief configuration struct
 * @version 
 * @date 2022-04-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <string>
#include <vector>

#include "core/video_format.h"

namespace seeder
{
    struct channel_config
    {
        int device_id;
        std::string video_mode;
        std::string multicast_ip;
        short multicast_port;
        std::string bind_ip;
        short bind_port;
        bool display_screen;
        core::video_format_desc format_desc;
        int audio_channels;
        int audio_samples;
        int audio_rate;
        int rtp_video_type;
        int rtp_audio_type;
        int leap_seconds; // 37s: leap second, since 1970-01-01 to 2022-01-01
    };

    struct config
    {
        std::string console_level;
        std::string file_level;
        int max_size;
        int max_files;
        std::string log_path;
        std::vector<channel_config> channels;
        int leap_seconds;
    };

}// namespace seeder