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

namespace seeder::util
{
    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to rgba 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_rgba(uint8_t* ycbcr422, seeder::core::video_format_desc& format_desc);

    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to rgba 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_bgra(uint8_t* ycbcr422, seeder::core::video_format_desc& format_desc);

    /**
     * @brief convert ycbcr422/uyvy422/cby0cry1 10bit to rgba 8 bit
     * 
     * @param ycbcr422 
     * @param format_desc 
     * @return std::shared_ptr<buffer> 
     */
    std::shared_ptr<buffer> ycbcr422_to_uyvy(uint8_t* ycbcr422, seeder::core::video_format_desc& format_desc);
    
} // namespace seeder::util