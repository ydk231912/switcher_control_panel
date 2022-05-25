/**
 * @file video_format.h
 * @author 
 * @brief video format related information
 * @version 
 * @date 2022-04-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <string>
#include <vector>
#include <boost/rational.hpp>

namespace seeder::core
{
    enum class video_format
    {
        pal,
        ntsc,
        x576p2500,
        x720p2500,
        x720p5000,
        x720p2398,
        x720p2400,
        x720p2997,
        x720p5994,
        x720p3000,
        x720p6000,
        x1080p2398,
        x1080p2400,
        x1080i5000,
        x1080i5994,
        x1080i6000,
        x1080p2500,
        x1080p2997,
        x1080p3000,
        x1080p5000,
        x1080p5994,
        x1080p6000,
        x1556p2398,
        x1556p2400,
        x1556p2500,
        x2160p2398,
        x2160p2400,
        x2160p2500,
        x2160p2997,
        x2160p3000,
        x2160p5000,
        x2160p5994,
        x2160p6000,
        invalid,
        count
    };

    struct video_format_desc
    {
        video_format         format;
        int                  width;
        int                  height;
        int                  square_width;
        int                  square_height;
        int                  field_count;
        double               fps; // actual framerate = duration/time_scale, e.g. i50 = 25 fps, p50 = 50 fps
        boost::rational<int> framerate;
        int                  time_scale;
        int                  duration;
        std::size_t          size; // frame size in bytes
        std::string          name; // name of output format

        int                  audio_channels = 8;
        int                  audio_sample_rate;
        std::vector<int>     audio_cadence; // rotating optimal number of samples per frame

        video_format_desc(video_format           format,
                        int              field_count,
                        int              width,
                        int              height,
                        int              square_width,
                        int              square_height,
                        int              time_scale,
                        int              duration,
                        std::string      name,
                        std::vector<int> audio_cadence);

        video_format_desc(std::string format_name);
        video_format_desc();
    };
}// namespace seeder::core