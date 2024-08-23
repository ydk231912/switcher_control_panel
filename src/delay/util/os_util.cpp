#include "os_util.h"

#include <boost/process.hpp>

#include <core/util/logger.h>


using seeder::core::logger;

namespace bp = boost::process;


namespace seeder {
namespace util {


// 关闭计算机
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