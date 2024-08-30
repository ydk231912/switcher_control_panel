#pragma once

#include <memory>
#include <system_error>
#include <vector>

#include <boost/asio/buffer.hpp>


namespace seeder {

enum class SliceError {
    Success = 0,
    NoSlice,
    NotOpened,
    NoData,
    InvalidArgument,
    InvalidState,
};

class SliceErrorCategory : public std::error_category
{
public:
    const char* name() const noexcept override;
    std::string message(int ev) const override;

    static const SliceErrorCategory & instance();

private:
    SliceErrorCategory() = default;
};

std::error_code make_error_code(SliceError e);

class SliceReader;

class SliceManager {
public:
    struct Config {
        std::string data_dir;
        std::string slice_file_prefix;
        size_t max_read_slice_count = 0;
        size_t slice_first_block_offset = 0;
        size_t slice_block_size = 0;
        size_t slice_block_max_count = 0;
        uint32_t preopen_write_slice_count = 3;
        uint32_t preopen_read_slice_count = 3;
        std::string file_mode;
        int io_threads = 1;
    };

    explicit SliceManager(const Config &in_config);

    ~SliceManager();

    void init();

    std::error_code write_slice_block(const boost::asio::const_buffer &buffer, size_t block_offset = 0);
    std::error_code write_slice_block(const std::vector<boost::asio::const_buffer> &buffers, size_t block_offset = 0);

    std::error_code read_slice_block(SliceReader *in_reader, const boost::asio::mutable_buffer &dst_buffer, size_t block_offset = 0);
    std::error_code read_slice_block(SliceReader *in_reader, const std::vector<boost::asio::mutable_buffer> &dst_buffers, size_t block_offset = 0);

    SliceReader * add_read_cursor(uint32_t read_cursor);

    bool remove_read_cursor(SliceReader *in_reader);

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder

namespace std {
    template <> 
    struct is_error_code_enum<seeder::SliceError> : true_type
    {
    };
} // namespace std