/**
 * @file logger.h
 * @author your name (you@domain.com)
 * @brief spdlog configure
 * @version 0.1
 * @date 2022-04-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "spdlog/spdlog.h"

namespace seeder::util
{
    // global variable
    extern std::shared_ptr<spdlog::logger> logger;

    std::shared_ptr<spdlog::logger> create_logger();

    std::shared_ptr<spdlog::logger> create_logger(std::string console_level,
                                                  std::string file_leve,
                                                  std::string log_path,
                                                  int max_size,
                                                  int max_files);
}