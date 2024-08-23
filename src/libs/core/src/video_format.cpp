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

#include "core/video_format.h"
#include <unordered_map>
#include <chrono>

namespace seeder::core
{
    const std::vector<video_format_desc> format_descs = {
        {video_fmt::pal, 2, 720, 576, 1024, 576, 50000, 1000, "PAL", "PAL", {1920 / 2}},
        {video_fmt::ntsc, 2, 720, 486, 720, 540, 60000, 1001, "NTSC", "NTSC", {801, 801, 801, 800, 801, 801, 801, 800, 801, 801}},
        {video_fmt::x576p2500, 1, 720, 576, 1024, 576, 25000, 1000, "576p2500", "576p25", {1920}},
        {video_fmt::x720p2398, 1, 1280, 720, 1280, 720, 24000, 1001, "720p2398", "720p2398", {2002}},
        {video_fmt::x720p2400, 1, 1280, 720, 1280, 720, 24000, 1000, "720p2400", "720p24", {2000}},
        {video_fmt::x720p2500, 1, 1280, 720, 1280, 720, 25000, 1000, "720p2500", "720p25", {1920}},
        {video_fmt::x720p5000, 1, 1280, 720, 1280, 720, 50000, 1000, "720p5000", "720p50", {960}},
        {video_fmt::x720p2997, 1, 1280, 720, 1280, 720, 30000, 1001, "720p2997", "720p2997", {1602, 1601, 1602, 1601, 1602}},
        {video_fmt::x720p5994, 1, 1280, 720, 1280, 720, 60000, 1001, "720p5994", "720p5994", {801, 800, 801, 801, 801}},
        {video_fmt::x720p3000, 1, 1280, 720, 1280, 720, 30000, 1000, "720p3000", "720p30", {1600}},
        {video_fmt::x720p6000, 1, 1280, 720, 1280, 720, 60000, 1000, "720p6000", "720p60", {800}},
        {video_fmt::x1080p2398, 1, 1920, 1080, 1920, 1080, 24000, 1001, "1080p2398", "1080p2398", {2002}},
        {video_fmt::x1080p2400, 1, 1920, 1080, 1920, 1080, 24000, 1000, "1080p2400", "1080p24", {2000}},
        {video_fmt::x1080i5000, 2, 1920, 1080, 1920, 1080, 50000, 1000, "1080i5000", "1080i50", {1920 / 2}},
        {video_fmt::x1080i5994, 2, 1920, 1080, 1920, 1080, 60000, 1001, "1080i5994", "1080i5994", {801, 801, 801, 800, 801, 801, 801, 800, 801, 801}},
        {video_fmt::x1080i6000, 2, 1920, 1080, 1920, 1080, 60000, 1000, "1080i6000", "1080i60", {1600 / 2}},
        {video_fmt::x1080p2500, 1, 1920, 1080, 1920, 1080, 25000, 1000, "1080p2500", "1080p25", {1920}},
        {video_fmt::x1080p2997, 1, 1920, 1080, 1920, 1080, 30000, 1001, "1080p2997", "1080p2997", {1602, 1601, 1602, 1601, 1602}},
        {video_fmt::x1080p3000, 1, 1920, 1080, 1920, 1080, 30000, 1000, "1080p3000", "1080p30", {1600}},
        {video_fmt::x1080p5000, 1, 1920, 1080, 1920, 1080, 50000, 1000, "1080p5000", "1080p50", {960}},
        {video_fmt::x1080p5994, 1, 1920, 1080, 1920, 1080, 60000, 1001, "1080p5994", "1080p5994", {801, 800, 801, 801, 801}},
        {video_fmt::x1080p6000, 1, 1920, 1080, 1920, 1080, 60000, 1000, "1080p6000", "1080p60", {800}},
        {video_fmt::x1556p2398, 1, 2048, 1556, 2048, 1556, 24000, 1001, "1556p2398", "1556p2398", {2002}},
        {video_fmt::x1556p2400, 1, 2048, 1556, 2048, 1556, 24000, 1000, "1556p2400", "1556p24", {2000}},
        {video_fmt::x1556p2500, 1, 2048, 1556, 2048, 1556, 25000, 1000, "1556p2500", "1556p25", {1920}},
        {video_fmt::x2160p2398, 1, 3840, 2160, 3840, 2160, 24000, 1001, "2160p2398", "2160p2398", {2002}},
        {video_fmt::x2160p2400, 1, 3840, 2160, 3840, 2160, 24000, 1000, "2160p2400", "2160p24", {2000}},
        {video_fmt::x2160p2500, 1, 3840, 2160, 3840, 2160, 25000, 1000, "2160p2500", "2160p25", {1920}},
        {video_fmt::x2160p2997, 1, 3840, 2160, 3840, 2160, 30000, 1001, "2160p2997", "2160p2997", {1602, 1601, 1602, 1601, 1602}},
        {video_fmt::x2160p3000, 1, 3840, 2160, 3840, 2160, 30000, 1000, "2160p3000", "2160p30", {1600}},
        {video_fmt::x2160p5000, 1, 3840, 2160, 3840, 2160, 50000, 1000, "2160p5000", "2160p50", {960}},
        {video_fmt::x2160p5994, 1, 3840, 2160, 3840, 2160, 60000, 1001, "2160p5994", "2160p5994", {801, 800, 801, 801, 801}},
        {video_fmt::x2160p6000, 1, 3840, 2160, 3840, 2160, 60000, 1000, "2160p6000", "2160p60", {800}},
        {video_fmt::invalid, 1, 0, 0, 0, 0, 1, 1, "invalid", "invalid", {1}}
    };

    video_format_desc::video_format_desc(video_fmt           format,
                               int              field_count,
                               int              width,
                               int              height,
                               int              square_width,
                               int              square_height,
                               int              time_scale,
                               int              duration,
                               const std::string      &name,
                               const std::string      &_display_name,
                               std::vector<int> audio_cadence)
    : format(format)
    , width(width)
    , height(height)
    , square_width(square_width)
    , square_height(square_height)
    , field_count(field_count)
    , fps(static_cast<double>(time_scale) / static_cast<double>(duration))
    , framerate(time_scale, duration) // boost::rational会自动normalize， 50000,1000 会变成 50,1
    , time_scale(time_scale)
    , duration(duration)
    , size(width * height * 4)
    , name(name), display_name(_display_name)
    , audio_sample_rate(48000)
    , audio_cadence(std::move(audio_cadence))
    {
    }
    

    video_format_desc::video_format_desc()
    {
        *this = format_descs.at(static_cast<int>(video_fmt::invalid));
    }

    static std::unordered_map<video_fmt, video_format_desc> init_format_map() {
        std::unordered_map<video_fmt, video_format_desc> fmt_map;
        for (auto &f : format_descs) {
            fmt_map.emplace(f.format, f);
        }
        return fmt_map;
    }

    const video_format_desc & video_format_desc::get(video_fmt f) {
        static const std::unordered_map<video_fmt, video_format_desc> format_map = init_format_map();
        static const video_format_desc invalid_instance;
        auto it = format_map.find(f);
        if (it != format_map.end()) {
            return it->second;
        }
        return invalid_instance;
    }

    static std::unordered_map<std::string, video_format_desc> init_format_name_map() {
        std::unordered_map<std::string, video_format_desc> fmt_map;
        for (auto &f : format_descs) {
            fmt_map.emplace(f.name, f);
        }
        return fmt_map;
    }

    const video_format_desc & video_format_desc::get(const std::string &name) {
        static const std::unordered_map<std::string, video_format_desc> format_map = init_format_name_map();
        static const video_format_desc invalid_instance;
        auto it = format_map.find(name);
        if (it != format_map.end()) {
            return it->second;
        }
        return invalid_instance;
    }

    double video_format_desc::ns_per_frame() const {
        int fps = framerate.denominator() == 1001 ? framerate.numerator() / 1000 : framerate.numerator();
        if (fps == 0) {
            return 0;
        }
        double ns_per_frame = static_cast<double>(std::nano::den) / static_cast<double>(fps);
        return ns_per_frame;
    }



} // namespace seeder::core