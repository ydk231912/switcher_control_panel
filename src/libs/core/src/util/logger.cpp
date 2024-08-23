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
#include "core/util/logger.h"

#include <unordered_map>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <spdlog/details/registry.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace {

std::mutex logger_init_mutex;

std::vector<spdlog::sink_ptr> & get_logger_init_sinks() {
    static std::vector<spdlog::sink_ptr> logger_init_sinks;
    return logger_init_sinks;
}

std::unordered_map<std::string, spdlog::level::level_enum> & get_logger_init_levels() {
    static std::unordered_map<std::string, spdlog::level::level_enum> logger_init_levels;
    return logger_init_levels;
}

spdlog::level::level_enum parsed_global_level = spdlog::level::level_enum::off;

void setup_logger_level(spdlog::logger &logger) {
    auto &name = logger.name();
    auto level_iter = get_logger_init_levels().find(name);
    if (level_iter != get_logger_init_levels().end()) {
        logger.set_level(level_iter->second);
    } else {
        logger.set_level(parsed_global_level);
    }
}

} // namespace

namespace seeder::core
{
    std::shared_ptr<spdlog::logger> logger = get_logger("seeder");

    std::shared_ptr<spdlog::logger> get_logger(const std::string &name) {
        auto logger = spdlog::get(name);
        if (logger) {
            return logger;
        }
        std::lock_guard<std::mutex> lock(logger_init_mutex);
        logger = std::make_shared<spdlog::logger>(name, get_logger_init_sinks().begin(), get_logger_init_sinks().end());
        spdlog::initialize_logger(logger);
        setup_logger_level(*logger);
        return logger;
    }

namespace logger_init {
    void setup_sinks(std::vector<spdlog::sink_ptr> &&sinks) {
        std::lock_guard<std::mutex> lock(logger_init_mutex);
        get_logger_init_sinks() = std::move(sinks);
        spdlog::details::registry::instance().apply_all([] (auto logger) {
            logger->sinks() = get_logger_init_sinks();
        });
    }

    void setup_levels(const std::string &levels_str) {
        std::lock_guard<std::mutex> lock(logger_init_mutex);
        // 参考
        // spdlog 新版本的 spdlog::cfg::helpers::load_levels
        // 1.5版本没有
        std::vector<std::string> key_values;
        boost::split(key_values, levels_str, boost::is_any_of(","));
        auto log_level_begin = key_values.begin();
        if (key_values.size() >= 1 && key_values.front().find("=") == std::string::npos) {
            // global level
            parsed_global_level = spdlog::level::from_str(key_values.front());
            spdlog::details::registry::instance().set_level(parsed_global_level);
            log_level_begin++;
        }
        auto &logger_init_levels = get_logger_init_levels();
        for (auto iter = log_level_begin; iter != key_values.end(); ++iter) {
            auto sep_pos = iter->find("=");
            if (sep_pos == std::string::npos || !sep_pos || sep_pos == iter->size() - 1) {
                continue;
            }
            auto key = iter->substr(0, sep_pos);
            auto value = iter->substr(sep_pos + 1);
            logger_init_levels[key] = spdlog::level::from_str(value);
        }
        spdlog::details::registry::instance().apply_all([] (auto logger) {
            setup_logger_level(*logger);
        });
    }
    
} // namespace seeder::core::logger_init

    // std::shared_ptr<spdlog::logger> create_logger()
    // {
    //     auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    //     console_sink->set_level(spdlog::level::debug);
    //     std::vector< spdlog::sink_ptr> sinks = {console_sink};
    //     auto logger = std::make_shared<spdlog::logger>("SEEDER ST2110", sinks.begin(), sinks.end());
    //     return logger;
    // }
    // std::shared_ptr<spdlog::logger> create_logger(std::string console_level,
    //                                               std::string file_leve,
    //                                               std::string log_path,
    //                                               int max_size,
    //                                               int max_files)
    // {
    //     auto size = 1048576 * max_size ; //MB

    //     auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    //     console_sink->set_level(spdlog::level::from_str(console_level));
        
    //     auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path, size, max_files);
    //     file_sink->set_level(spdlog::level::from_str(file_leve));
        
    //     std::vector< spdlog::sink_ptr> sinks = {console_sink, file_sink};
    //     auto logger = std::make_shared<spdlog::logger>("SEEDER ST2110", sinks.begin(), sinks.end());
    //     //logger->set_level(spdlog::level::debug);
    //     //logger->set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %s"); //https://github.com/gabime/spdlog/wiki/3.-Custom-formatting

    //     return logger;
    // }
}