/**
 * @file rtp_context.h
 * @author 
 * @brief 
 * @version 0.1
 * @date 2022-07-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "core/video_format.h"

using namespace seeder::core;
namespace seeder::rtp
{
    struct rtp_context
    {
        rtp_context(video_format_desc desc, std::string ip, short port)
        {
            format_desc = desc;
            multicast_ip = ip;
            multicast_port = port;
        }
        
        video_format_desc format_desc;
        std::string multicast_ip;
        short multicast_port;
    };
}