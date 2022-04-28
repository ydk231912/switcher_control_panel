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

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "logger.h"

namespace seeder::util
{
    std::shared_ptr<spdlog::logger> logger = create_logger();

    std::shared_ptr<spdlog::logger> create_logger()
    {
        auto max_size = 1048576 * 10 ; //10M
        auto max_files = 30;

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("/home/seeder/logs/st2110/log.txt", max_size, max_files);
        file_sink->set_level(spdlog::level::warn); //only write error and warn log to file
        
        std::vector< spdlog::sink_ptr> sinks = {console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("SEEDER ST2110", sinks.begin(), sinks.end());
        //logger->set_level(spdlog::level::debug);

        return logger;
    }
    
}