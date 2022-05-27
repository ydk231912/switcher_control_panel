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

#pragma once

#include <memory>

#include "buffer.h"
#include "core/video_format.h"

using namespace seeder::core;
namespace seeder::util
{
    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to rgba 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_rgba(std::shared_ptr<buffer> ycbcr422, video_format_desc& format_desc);

    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to rgba 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_bgra(std::shared_ptr<buffer> ycbcr422, video_format_desc& format_desc);

    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to rgba 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_uyvy(std::shared_ptr<buffer> ycbcr422, video_format_desc& format_desc);
    
} // namespace seeder::util