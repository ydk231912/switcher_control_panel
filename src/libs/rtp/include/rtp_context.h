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
            rtp_video_type = 96;
            rtp_audio_type = 97;
            leap_seconds = 37;
        }
        
        rtp_context(video_format_desc desc, std::string ip, short port
        ,uint8_t video_type, uint8_t audio_type, uint32_t leap)
        {
            format_desc = desc;
            multicast_ip = ip;
            multicast_port = port;
            rtp_video_type = video_type;
            rtp_audio_type = audio_type;
            leap_seconds = leap;
        }

        video_format_desc format_desc;
        std::string multicast_ip;
        short multicast_port;

        uint8_t rtp_video_type;
        uint8_t rtp_audio_type;
        int32_t leap_seconds; 
    };
}