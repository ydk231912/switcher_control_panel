# pragma once

#include <system_error>
#include <vector>
#include <string>
#include <core/util/buffer.h>


namespace seeder {

namespace util {

std::error_code read_file(const std::string &file_path, std::shared_ptr<seeder::core::AbstractBuffer> &out_buffer);

std::error_code write_file(const std::string &file_path, const uint8_t *data, std::size_t size);

std::error_code write_file(const std::string &file_path, std::vector<std::shared_ptr<seeder::core::AbstractBuffer>> buffers);

} // namespace seeder::util

} // namespace seeder