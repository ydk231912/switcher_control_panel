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
    enum class video_fmt
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
        video_fmt         format = video_fmt::invalid;
        int                  width = 0;
        int                  height = 0;
        int                  square_width = 0;
        int                  square_height = 0;
        int                  field_count = 1;
        double               fps = 0; // actual framerate = duration/time_scale, e.g. i50 = 25 fps, p50 = 50 fps
        boost::rational<int> framerate;
        int                  time_scale = 0;
        int                  duration = 0;
        std::size_t          size = 0; // frame size in bytes
        std::string          name; // name of output format
        std::string          display_name;

        int                  audio_channels = 2; // 声道数量
        int                  audio_sample_rate = 0; // 采样率 48000 96000
        int                  audio_samples = 0; // 位宽： 16 = 16bit 24 = 24bit
        std::vector<int>     audio_cadence; // rotating optimal number of samples per frame
        int st30_frame_size = 0;  // st30 audio frame size = 位宽字节数(audio_samples/8) * 声道数量(audio_channels) * 每帧采样数量(sample_num)
        int st30_fps = 0; // st30 audio frame fps
        double st30_ptime = 0;
        int sample_num = 0; // 每帧(每声道)采样数量

        video_format_desc(video_fmt           format,
                        int              field_count,
                        int              width,
                        int              height,
                        int              square_width,
                        int              square_height,
                        int              time_scale,
                        int              duration,
                        const std::string      &name,
                        const std::string      &display_name,
                        std::vector<int> audio_cadence);
        
        video_format_desc();

        bool is_invalid() const {
            return format == video_fmt::invalid;
        }

        static const video_format_desc & get(video_fmt f);

        static const video_format_desc & get(const std::string &name);

        double ns_per_frame() const;
    };

    extern const std::vector<video_format_desc> format_descs;
}// namespace seeder::core