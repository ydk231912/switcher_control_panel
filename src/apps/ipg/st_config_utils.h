#include "app_base.h"
#include "app_session.h"
#include "core/util/logger.h"
#include "parse_json.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <fstream>

namespace seeder {
namespace utils {
void write_string_to_file_directly(const std::string &file_path,
                                   const std::string &content);

void write_string_to_file(const std::string &file_path,
                          const std::string &content);

std::string read_file_to_string(const std::string &file_path);

void save_config_file(st_app_context *app_ctx);
} // namespace utils

} // namespace seeder
