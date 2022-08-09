/**
 * @file png_writer.h
 * @author 
 * @brief 
 * @version 1.0
 * @date 2022-08-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <string>

namespace seeder::core
{
    /**
     * @brief write frame data to png file
     * 
     * @param data  RGBA 8 bit
     * @param width 
     * @param height 
     * @param path 
     */
    void write_png(uint8_t* data, int width, int height, std::string path);
}