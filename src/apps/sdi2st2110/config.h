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

#include "util/video_format.h"

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
        util::video_format_desc format_desc;
    };

    struct config
    {
        std::string console_level;
        std::string file_level;
        int max_size;
        int max_files;
        std::string log_path;
        std::vector<channel_config> channels;
    };

}// namespace seeder