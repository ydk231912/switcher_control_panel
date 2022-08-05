/**
 * @file color_conversion.h
 * @author 
 * @brief color conversion, for example, uyvy10 to rgba, uyvy10 to bgra, uyvy10 to uyvy8
 * @version 
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <math.h>
#include <tuple>

#include "core/util/color_conversion.h"

namespace seeder::core
{
    uint8_t clip_8(double v) 
    { 
        return static_cast<uint8_t>(round(std::max(0.0, std::min(255.0, v)))); 
    }

    std::tuple<uint8_t, uint8_t, uint8_t> ycbcr_to_rgb_rec709(int y, int cb, int cr)
    {
        auto Y  = static_cast<double>(y - 16);
        auto Cb = static_cast<double>(cb - 128);
        auto Cr = static_cast<double>(cr - 128);

        auto r = 1.16438356164384 * Y + 1.79274107142857 * Cr;
        auto g = 1.16438356164384 * Y - 0.21324861427373 * Cb - 0.532909328559444 * Cr;
        auto b = 1.16438356164384 * Y + 2.11240178571429 * Cb;

        return {clip_8(r), clip_8(g), clip_8(b)};
    }

    std::tuple<uint8_t, uint8_t, uint8_t> ycbcr_to_rgb_rec2020(int y, int cb, int cr)
    {
        auto Y  = static_cast<double>(y - 16);
        auto Cb = static_cast<double>(cb - 128);
        auto Cr = static_cast<double>(cr - 128);

        auto r = 1.16438356164384 * Y + 1.67867410714286 * Cr;
        auto g = 1.16438356164384 * Y - 0.187326104219343 * Cb - 0.650424318505057 * Cr;
        auto b = 1.16438356164384 * Y + 2.14177232142857 * Cb;

        return {clip_8(r), clip_8(g), clip_8(b)};
    }

    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to rgba 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_rgba(uint8_t* ycbcr422, video_format_desc& format_desc)
    {
        const auto width                 = format_desc.width;
        const auto height                = format_desc.height;
        const auto rgba_line_size        = width * 4;
        const auto rgba_frame_size       = height * rgba_line_size;
        constexpr auto pixels_per_pgroup = 2;
        const auto ycbcr_frame_size = (width * 5 / 2) * height; // 2 pixel = 40bit

        auto converter = format_desc.height > 1080 ? ycbcr_to_rgb_rec2020 : ycbcr_to_rgb_rec709;

        std::shared_ptr<buffer> frame_buffer = std::make_shared<buffer>(ycbcr_frame_size);

        auto input = ycbcr422;

        for(auto y = 0; y < height; ++y)
        {
            auto output = frame_buffer->begin() + y * rgba_line_size;

            for(auto x = 0; x < width; x += pixels_per_pgroup)
            {
                //      Cb         Y0        Cr         Y1
                // XXXXXXXX XX++++++ ++++XXXX XXXXXX++ ++++++++
                //     0        1        2        3        4

                auto cb = static_cast<uint8_t>(input[0]);

                auto y0 = static_cast<uint8_t>((input[1] & uint8_t(0x3F)) << 2);
                y0 |= static_cast<uint8_t>((input[2] & uint8_t(0xC0)) >> 6);

                auto cr = static_cast<uint8_t>((input[2] & uint8_t(0x0F)) << 4);
                cr |= static_cast<uint8_t>((input[3] & uint8_t(0xF0)) >> 4);

                // Y1
                auto y1 = static_cast<uint8_t>((input[3] & uint8_t(0x03)) << 6);
                y1 |= static_cast<uint8_t>((input[4] & uint8_t(0xFC)) >> 2);

                const auto [r1, g1, b1] = converter(y0, cb, cr);
                const auto [r0, g0, b0] = converter(y0, cb, cr);

                output[0] = uint8_t(r0);
                output[1] = uint8_t(g0);
                output[2] = uint8_t(b0);
                output[3] = uint8_t(0xFF);
                output[4] = uint8_t(r1);
                output[5] = uint8_t(g1);
                output[6] = uint8_t(b1);
                output[7] = uint8_t(0xFF);

                input += 5;
                output += 8;
            }
        }

        return frame_buffer;
    }

    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to rgba 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_bgra(uint8_t* ycbcr422, video_format_desc& format_desc)
    {
        const auto width                 = format_desc.width;
        const auto height                = format_desc.height;
        const auto rgba_line_size        = width * 4;
        const auto rgba_frame_size       = height * rgba_line_size;
        constexpr auto pixels_per_pgroup = 2;
        const auto ycbcr_frame_size = (width * 5 / 2) * height; // 2 pixel = 40bit

        auto converter = format_desc.height > 1080 ? ycbcr_to_rgb_rec2020 : ycbcr_to_rgb_rec709;

        std::shared_ptr<buffer> frame_buffer = std::make_shared<buffer>(ycbcr_frame_size);

        auto input = ycbcr422;

        for(auto y = 0; y < height; ++y)
        {
            auto output = frame_buffer->begin() + y * rgba_line_size;

            for(auto x = 0; x < width; x += pixels_per_pgroup)
            {
                //      Cb         Y0        Cr         Y1
                // XXXXXXXX XX++++++ ++++XXXX XXXXXX++ ++++++++
                //     0        1        2        3        4

                auto cb = static_cast<uint8_t>(input[0]);

                auto y0 = static_cast<uint8_t>((input[1] & uint8_t(0x3F)) << 2);
                y0 |= static_cast<uint8_t>((input[2] & uint8_t(0xC0)) >> 6);

                auto cr = static_cast<uint8_t>((input[2] & uint8_t(0x0F)) << 4);
                cr |= static_cast<uint8_t>((input[3] & uint8_t(0xF0)) >> 4);

                // Y1
                auto y1 = static_cast<uint8_t>((input[3] & uint8_t(0x03)) << 6);
                y1 |= static_cast<uint8_t>((input[4] & uint8_t(0xFC)) >> 2);

                const auto [r1, g1, b1] = converter(y0, cb, cr);
                const auto [r0, g0, b0] = converter(y0, cb, cr);

                output[0] = uint8_t(b0);
                output[1] = uint8_t(g0);
                output[2] = uint8_t(r0);
                output[3] = uint8_t(0xFF);
                output[4] = uint8_t(b1);
                output[5] = uint8_t(g1);
                output[6] = uint8_t(r1);
                output[7] = uint8_t(0xFF);

                input += 5;
                output += 8;
            }
        }

        return frame_buffer;
    }

    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_uyvy(uint8_t* ycbcr422, video_format_desc& format_desc)
    {
        const auto width           = format_desc.width;
        const auto height          = format_desc.height;
        const auto uyvy_line_size  = width * 2;
        const auto uyvy_frame_size = height * uyvy_line_size;
        const auto ycbcr_frame_size = (width * 5 / 2) * height; // 2 pixel = 40bit
        // const auto input_stride = width * 5 / 2;
        constexpr auto pixels_per_pgroup = 2;

        std::shared_ptr<buffer> frame_buffer = std::make_shared<buffer>(ycbcr_frame_size);

        auto input = ycbcr422;

        for(auto y = 0; y < height; ++y)
        {
            auto output = frame_buffer->begin() + y * uyvy_line_size;

            for(auto x = 0; x < width; x += pixels_per_pgroup)
            {
                //      Cb         Y0        Cr         Y1
                // XXXXXXXX XX++++++ ++++XXXX XXXXXX++ ++++++++
                //     0        1        2        3        4

                // Cb
                *output = static_cast<uint8_t>(input[0] >> 2); // is it right? 
                ++output;

                // Y0
                *output = static_cast<uint8_t>((input[1] & uint8_t(0x3F)) << 2);
                *output |= static_cast<uint8_t>((input[2] & uint8_t(0xC0)) >> 6);
                ++output;

                // Cr
                *output = static_cast<uint8_t>((input[2] & uint8_t(0x0F)) << 4);
                *output |= static_cast<uint8_t>((input[3] & uint8_t(0xF0)) >> 4);
                ++output;

                // Y1
                *output = static_cast<uint8_t>((input[3] & uint8_t(0x03)) << 6);
                *output |= static_cast<uint8_t>((input[4] & uint8_t(0xFC)) >> 2);
                ++output;

                input += 5;
            }
        }

        return frame_buffer;
    }
    
} // namespace seeder::core