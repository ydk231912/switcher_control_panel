#include "core/util/fs.h"

#include <fstream>
#include <system_error>

namespace seeder {

namespace core {

std::error_code read_file_to_string(const std::string &file_path, std::string &output) {
    std::fstream stream(file_path, std::ios_base::in);
    if (!stream) {
        return std::error_code(errno, std::system_category());
    }
    std::string buf;
    while (stream >> buf) {
        output.append(buf);
    }
    return {};
}

} // namespace core

} // namespace seeder