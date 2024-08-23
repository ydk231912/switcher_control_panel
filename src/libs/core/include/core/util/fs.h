#pragma once

#include <system_error>

namespace seeder {

namespace core {

std::error_code read_file_to_string(const std::string &file_path, std::string &output);

} // namespace core

} // namespace seeder