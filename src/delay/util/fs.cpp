#include "fs.h"

namespace {

class FileDeleter {
public:
    void operator()(FILE *file) {
        fclose(file);
    }
};

} // namepsace

namespace seeder {

namespace util {

std::error_code read_file(const std::string &file_path, std::shared_ptr<seeder::core::AbstractBuffer> &out_buffer) {
    std::unique_ptr<FILE, FileDeleter> file(fopen(file_path.c_str(), "rb"));
    if (!file) {
        return std::make_error_code(std::errc(errno));
    }
    if (fseek(file.get(), 0, SEEK_END)) {
        return std::make_error_code(std::errc(ferror(file.get())));
    }
    size_t file_size = ftell(file.get());
    if (fseek(file.get(), 0, SEEK_SET)) {
        return std::make_error_code(std::errc(ferror(file.get())));
    }
    auto buffer = seeder::core::alloc_plain_buffer(file_size);
    uint8_t *buffer_data = (uint8_t *)buffer->get_data();
    size_t offset = 0;
    while (offset < file_size) {
        offset += fread(buffer_data + offset, 1, file_size - offset, file.get());
        if (feof(file.get())) {
            break;
        } else if (ferror(file.get())) {
            return std::make_error_code(std::errc(ferror(file.get())));
            break;
        }
    }
    out_buffer = buffer;
    return {};
}

std::error_code write_file(const std::string &file_path, const uint8_t *data, std::size_t size) {
    std::unique_ptr<FILE, FileDeleter> file(fopen(file_path.c_str(), "wb"));
    if (!file) {
        return std::make_error_code(std::errc(errno));
    }
    fwrite(data, 1, size, file.get());
    if (ferror(file.get())) {
        return std::make_error_code(std::errc(ferror(file.get())));
    }
    return {};
}

std::error_code write_file(const std::string &file_path, std::vector<std::shared_ptr<seeder::core::AbstractBuffer>> buffers) {
    std::unique_ptr<FILE, FileDeleter> file(fopen(file_path.c_str(), "wb"));
    if (!file) {
        return std::make_error_code(std::errc(errno));
    }
    for (auto &buffer : buffers) {
        fwrite(buffer->get_data(), 1, buffer->get_size(), file.get());
        if (ferror(file.get())) {
            return std::make_error_code(std::errc(ferror(file.get())));
        }
    }
    return {};
}

} // namespace seeder::util

} // namespace seeder