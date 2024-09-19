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

struct SliceReaderOption {
    bool follow_write = false;
    bool always_reset_cursor = false;
};

struct SliceReadOption {
    size_t block_offset = 0;
    bool advance_cursor = true;
    bool fallback_last_block = false;
};

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

    std::error_code read_slice_block(SliceReader *in_reader, const boost::asio::mutable_buffer &dst_buffer) {
        SliceReadOption options;
        return this->read_slice_block(in_reader, dst_buffer, options);
    }

    std::error_code read_slice_block(SliceReader *in_reader, const boost::asio::mutable_buffer &dst_buffer, const SliceReadOption &options);

    std::error_code read_slice_block(SliceReader *in_reader, const std::vector<boost::asio::mutable_buffer> &dst_buffers) {
        SliceReadOption options;
        return this->read_slice_block(in_reader, dst_buffers, options);
    }

    std::error_code read_slice_block(SliceReader *in_reader, const std::vector<boost::asio::mutable_buffer> &dst_buffers, const SliceReadOption &options);

    SliceReader * add_read_cursor(uint32_t read_cursor, const SliceReaderOption &option);

    bool remove_read_cursor(SliceReader *in_reader);

    void set_slice_option(SliceReader *in_reader, const SliceReaderOption &option);

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