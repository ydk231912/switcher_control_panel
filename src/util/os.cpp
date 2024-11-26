#include "os.h"

#include <boost/process.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace bp = boost::process;

namespace seeder {

// Logger
extern std::shared_ptr<spdlog::logger> logger;

namespace util {

void restart() {
    logger->info("restart");
    try {
        std::string std_err;
        int exit_code = bp::system("restart", bp::std_err > std_err);
        if (exit_code) {
            logger->error("restart failed exit_code={} std_err={}", exit_code, std_err);
        }
    } catch (std::exception &e) {
        logger->error("restart exception: {}", e.what());
    }
}

void poweroff() {
    logger->info("poweroff");
    try {
        std::string std_err;
        int exit_code = bp::system("poweroff", bp::std_err > std_err);
        if (exit_code) {
            logger->error("poweroff failed exit_code={} std_err={}", exit_code, std_err);
        }
    } catch (std::exception &e) {
        logger->error("poweroff exception: {}", e.what());
    }
}

} // namespace seeder::util
} // namespace seeder