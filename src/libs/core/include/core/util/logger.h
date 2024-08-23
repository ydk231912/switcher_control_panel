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

#pragma once

#include "spdlog/logger.h"
#include "spdlog/fmt/ostr.h"

namespace seeder::core
{
    // global variable
    extern std::shared_ptr<spdlog::logger> logger;

    /** 获取logger
    若logger不存在，则会创建一个新的logger并返回
    创建的logger的sinks为setup_sinks()中的sinks
    */
    std::shared_ptr<spdlog::logger> get_logger(const std::string &name);

namespace logger_init {
    /** 设置logger的输出sinks
    */
    void setup_sinks(std::vector<spdlog::sink_ptr> &&sinks);

    /** Init levels from given string
    set global level to debug: "debug"
    turn off all logging except for logger1: "off,logger1=debug"
    turn off all logging except for logger1 and logger2: "off,logger1=debug,logger2=info"
    */
    void setup_levels(const std::string &levels_str);

} // namespace seeder::core::logger_init

    namespace impl {
        template<class T, class S>
        class Join {
        public:
            explicit Join(const T &_v, const S &_s): value(_v), sep(_s) {}

            friend std::ostream & operator<<(std::ostream &os, const Join &j) {
                bool is_first = true;
                for (const auto &v : j.value) {
                    if (!is_first) {
                        os << j.sep;
                    }
                    os << v;
                    is_first = false;
                }
                return os;
            }

        private:
            const T &value;
            const S &sep;
        };
    }

    template<class T, class S>
    impl::Join<T, S> join(const T &collection, const S &sep) {
        return impl::Join(collection, sep);
    }
}

template<class T, class S> struct fmt::formatter<seeder::core::impl::Join<T, S>> : fmt::ostream_formatter {};
