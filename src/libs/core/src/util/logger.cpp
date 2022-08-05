/**
 * @file logger.h
 * @author 
 * @brief spdlog configure
 * @version 
 * @date 2022-04-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "core/util/logger.h"

namespace seeder::core
{
    std::shared_ptr<spdlog::logger> logger = create_logger();

    std::shared_ptr<spdlog::logger> create_logger()
    {

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        std::vector< spdlog::sink_ptr> sinks = {console_sink};
        auto logger = std::make_shared<spdlog::logger>("SEEDER ST2110", sinks.begin(), sinks.end());
        return logger;
    }

    std::shared_ptr<spdlog::logger> create_logger(std::string console_level,
                                                  std::string file_leve,
                                                  std::string log_path,
                                                  int max_size,
                                                  int max_files)
    {
        auto size = 1048576 * max_size ; //MB

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::from_str(console_level));
        
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path, size, max_files);
        file_sink->set_level(spdlog::level::from_str(file_leve));
        
        std::vector< spdlog::sink_ptr> sinks = {console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("SEEDER ST2110", sinks.begin(), sinks.end());
        //logger->set_level(spdlog::level::debug);
        //logger->set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %s"); //https://github.com/gabime/spdlog/wiki/3.-Custom-formatting

        return logger;
    }
}