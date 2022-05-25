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

#include <boost/algorithm/string.hpp>
#include "video_format.h"

namespace seeder::core
{
    const std::vector<video_format_desc> format_descs = {
    {video_format::pal, 2, 720, 576, 1024, 576, 50000, 1000, "PAL", {1920 / 2}},
    {video_format::ntsc, 2, 720, 486, 720, 540, 60000, 1001, "NTSC", {801, 801, 801, 800, 801, 801, 801, 800, 801, 801}},
    {video_format::x576p2500, 1, 720, 576, 1024, 576, 25000, 1000, "576p2500", {1920}},
    {video_format::x720p2398, 1, 1280, 720, 1280, 720, 24000, 1001, "720p2398", {2002}},
    {video_format::x720p2400, 1, 1280, 720, 1280, 720, 24000, 1000, "720p2400", {2000}},
    {video_format::x720p2500, 1, 1280, 720, 1280, 720, 25000, 1000, "720p2500", {1920}},
    {video_format::x720p5000, 1, 1280, 720, 1280, 720, 50000, 1000, "720p5000", {960}},
    {video_format::x720p2997, 1, 1280, 720, 1280, 720, 30000, 1001, "720p2997", {1602, 1601, 1602, 1601, 1602}},
    {video_format::x720p5994, 1, 1280, 720, 1280, 720, 60000, 1001, "720p5994", {801, 800, 801, 801, 801}},
    {video_format::x720p3000, 1, 1280, 720, 1280, 720, 30000, 1000, "720p3000", {1600}},
    {video_format::x720p6000, 1, 1280, 720, 1280, 720, 60000, 1000, "720p6000", {800}},
    {video_format::x1080p2398, 1, 1920, 1080, 1920, 1080, 24000, 1001, "1080p2398", {2002}},
    {video_format::x1080p2400, 1, 1920, 1080, 1920, 1080, 24000, 1000, "1080p2400", {2000}},
    {video_format::x1080i5000, 2, 1920, 1080, 1920, 1080, 50000, 1000, "1080i5000", {1920 / 2}},
    {video_format::x1080i5994, 2, 1920, 1080, 1920, 1080, 60000, 1001, "1080i5994", {801, 801, 801, 800, 801, 801, 801, 800, 801, 801}},
    {video_format::x1080i6000, 2, 1920, 1080, 1920, 1080, 60000, 1000, "1080i6000", {1600 / 2}},
    {video_format::x1080p2500, 1, 1920, 1080, 1920, 1080, 25000, 1000, "1080p2500", {1920}},
    {video_format::x1080p2997, 1, 1920, 1080, 1920, 1080, 30000, 1001, "1080p2997", {1602, 1601, 1602, 1601, 1602}},
    {video_format::x1080p3000, 1, 1920, 1080, 1920, 1080, 30000, 1000, "1080p3000", {1600}},
    {video_format::x1080p5000, 1, 1920, 1080, 1920, 1080, 50000, 1000, "1080p5000", {960}},
    {video_format::x1080p5994, 1, 1920, 1080, 1920, 1080, 60000, 1001, "1080p5994", {801, 800, 801, 801, 801}},
    {video_format::x1080p6000, 1, 1920, 1080, 1920, 1080, 60000, 1000, "1080p6000", {800}},
    {video_format::x1556p2398, 1, 2048, 1556, 2048, 1556, 24000, 1001, "1556p2398", {2002}},
    {video_format::x1556p2400, 1, 2048, 1556, 2048, 1556, 24000, 1000, "1556p2400", {2000}},
    {video_format::x1556p2500, 1, 2048, 1556, 2048, 1556, 25000, 1000, "1556p2500", {1920}},
    {video_format::x2160p2398, 1, 3840, 2160, 3840, 2160, 24000, 1001, "2160p2398", {2002}},
    {video_format::x2160p2400, 1, 3840, 2160, 3840, 2160, 24000, 1000, "2160p2400", {2000}},
    {video_format::x2160p2500, 1, 3840, 2160, 3840, 2160, 25000, 1000, "2160p2500", {1920}},
    {video_format::x2160p2997, 1, 3840, 2160, 3840, 2160, 30000, 1001, "2160p2997", {1602, 1601, 1602, 1601, 1602}},
    {video_format::x2160p3000, 1, 3840, 2160, 3840, 2160, 30000, 1000, "2160p3000", {1600}},
    {video_format::x2160p5000, 1, 3840, 2160, 3840, 2160, 50000, 1000, "2160p5000", {960}},
    {video_format::x2160p5994, 1, 3840, 2160, 3840, 2160, 60000, 1001, "2160p5994", {801, 800, 801, 801, 801}},
    {video_format::x2160p6000, 1, 3840, 2160, 3840, 2160, 60000, 1000, "2160p6000", {800}},
    {video_format::invalid, 1, 0, 0, 0, 0, 1, 1, "invalid", {1}}};

    video_format_desc::video_format_desc(video_format           format,
                               int              field_count,
                               int              width,
                               int              height,
                               int              square_width,
                               int              square_height,
                               int              time_scale,
                               int              duration,
                               std::string      name,
                               std::vector<int> audio_cadence)
    : format(format)
    , width(width)
    , height(height)
    , square_width(square_width)
    , square_height(square_height)
    , field_count(field_count)
    , fps(static_cast<double>(time_scale) / static_cast<double>(duration))
    , framerate(time_scale, duration)
    , time_scale(time_scale)
    , duration(duration)
    , size(width * height * 4)
    , name(name)
    , audio_sample_rate(48000)
    , audio_cadence(std::move(audio_cadence))
    {
    }

    video_format_desc::video_format_desc(std::string name)
    {
        *this = format_descs.at(static_cast<int>(video_format::invalid));
        for (auto it = std::begin(format_descs); it != std::end(format_descs) - 1; ++it) 
        {
            if (boost::iequals(it->name, name)) 
            {
                *this = *it;
                break;
            }
        }
    }

    video_format_desc::video_format_desc()
    {
        *this = format_descs.at(static_cast<int>(video_format::invalid));
    }

} // namespace seeder::core