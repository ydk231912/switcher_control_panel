#include "slice_manager.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <deque>
#include <utility>
#include <system_error>

#include <fcntl.h>
#include <sys/mman.h>

#include <boost/assert.hpp>
#include <boost/align/align_up.hpp> 
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <tracy/Tracy.hpp>

#include "core/util/logger.h"


namespace asio = boost::asio;
namespace fs = boost::filesystem;


#define SYS_ERR_MSG(n) std::system_category().message(n)


namespace seeder {

std::error_code make_error_code(SliceError e) {
    return std::error_code(static_cast<int>(e), SliceErrorCategory::instance());
}

const char* SliceErrorCategory::name() const noexcept {
    return "SliceError";
}

const SliceErrorCategory & SliceErrorCategory::instance() {
    static SliceErrorCategory value;
    return value;
}

#define CASE_RETURN_MSG(name) case SliceError::name: return #name

std::string SliceErrorCategory::message(int ev) const {
    switch ((SliceError) ev) {
        CASE_RETURN_MSG(Success);
        CASE_RETURN_MSG(NoSlice);
        CASE_RETURN_MSG(NotOpened);
        CASE_RETURN_MSG(NoData);
        CASE_RETURN_MSG(InvalidArgument);
        CASE_RETURN_MSG(InvalidState);
    }
    return "";
}

namespace {

std::shared_ptr<spdlog::logger> logger = seeder::core::get_logger("slice");

struct FileCloseGuard {
    explicit FileCloseGuard(int in_fd): fd(in_fd) {}

    ~FileCloseGuard() {
        if (fd != -1) {
            close(fd);
        }
    }

    int fd = -1;
};

struct UnmapGuard {
    explicit UnmapGuard() {}
    explicit UnmapGuard(void *in_addr, size_t in_length): addr(in_addr), length(in_length) {}

    void release() {
        if (addr != MAP_FAILED) {
            ZoneScopedN("munmap");
            munmap(addr, length);

            addr = MAP_FAILED;
            length = 0;
        }
    }

    ~UnmapGuard() {
        release();
    }

    void *addr = MAP_FAILED;
    size_t length = 0;
};

struct SliceFile {

    void open(size_t in_file_size, bool truncate) {
        ZoneScopedN("open");
        auto lock = acquire_lock();
        if (is_opened) {
            return;
        }

        file_size = in_file_size;

        int fd = ::open(file_path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            logger->error("open {} error: {}", file_path, SYS_ERR_MSG(errno));
            throw std::runtime_error("open");
        }
        FileCloseGuard fd_guard(fd);
        if (is_first_open && truncate) {
            if (ftruncate(fd, file_size) == -1) {
                logger->error("ftruncate error: {}", SYS_ERR_MSG(errno));
                throw std::runtime_error("ftruncate");
            }
        }
        {
            ZoneScopedN("mmap");
            int prot = PROT_READ | PROT_WRITE;
            int flags = MAP_SHARED | MAP_LOCKED | MAP_POPULATE;
            void *map_base = mmap(NULL, file_size, prot, flags, fd, 0);
            if (map_base == MAP_FAILED) {
                logger->error("mmap error: {}", SYS_ERR_MSG(errno));
                throw std::runtime_error("mmap");
            }
            unmap_guard.addr = map_base;
            unmap_guard.length = file_size;
        }
        is_opened = true;
        is_first_open = false;
        if (truncate) {
            writer_index = 0;
        }
    }

    void close() {
        ZoneScopedN("close");
        auto lock = acquire_lock();
        if (!is_opened) {
            return;
        }
        is_opened = false;
        unmap_guard.release();
    }

    void *data() {
        if (unmap_guard.addr != MAP_FAILED) {
            return unmap_guard.addr;
        }
        return NULL;
    }

    std::unique_lock<std::shared_mutex> acquire_lock() {
        return std::unique_lock<std::shared_mutex>(mutex);
    }

    std::shared_lock<std::shared_mutex> acquire_shared_lock() {
        return std::shared_lock<std::shared_mutex>(mutex);
    }

    std::shared_mutex mutex;
    std::atomic<bool> is_opened = false;
    bool is_first_open = true;
    bool is_writing = false;
    uint32_t writer_index = 0;
    std::string file_name;
    fs::path file_path;
    size_t file_size = 0;
    UnmapGuard unmap_guard;
};

} // namespace seeder::{}

class SliceReader {
public:
    std::shared_ptr<SliceFile> slice;
    uint32_t read_cursor = 0;
    uint32_t read_index = 0; // block index in slice
};

class SliceManager::Impl {
public:
    explicit Impl(
        size_t in_max_read_slice_count,
        size_t in_slice_first_block_offset,
        size_t in_slice_block_size,
        size_t in_slice_block_max_count
    ):
        max_slice_count(in_max_read_slice_count + preopen_write_slice_count + 1),
        max_read_slice_count(in_max_read_slice_count),
        slice_first_block_offset(in_slice_first_block_offset),
        slice_block_size(in_slice_block_size),
        slice_block_max_count(in_slice_block_max_count),
        // FIXME 这里用多线程反而会导致 write_slice_block 的 memcpy 变慢...
        thread_pool(1)
    {
        if (in_max_read_slice_count == 0) {
            throw std::invalid_argument("max_slice_count");
        }
    }

    void init(
        const std::string &slice_dir,
        const std::string &slice_file_prefix
    ) {
        for (int i = 0; i < max_slice_count; ++i) {
            std::string file_name = (boost::format("%s-%d") % slice_file_prefix % i).str();
            fs::path file_path = fs::path(slice_dir).append(file_name);
            std::shared_ptr<SliceFile> slice(new SliceFile);
            slice->file_path = file_path;
            slice->file_name = file_path.filename().string();
            free_slices.push_back(slice);
        }
        do_prepare_write_slice_async();
    }

    template<class ConstBufferSeq>
    std::error_code write_slice_block(const ConstBufferSeq &buffers, size_t block_offset) {
        ZoneScopedN("write_slice_block");
        auto lock = acquire_lock();
        if (!opened_write_slice) {
            return SliceError::NoSlice;
        }
        if (!opened_write_slice->is_opened) {
            return SliceError::NotOpened;
        }
        if (opened_write_slice->writer_index >= slice_block_max_count) {
            return SliceError::InvalidState;
        }
        if (block_offset + asio::buffer_size(buffers) > slice_block_size) {
            return SliceError::InvalidArgument;
        }
        if (opened_write_slice->is_writing) {
            return SliceError::InvalidState;
        }

        opened_write_slice->is_writing = true;
        lock.unlock();

        {
            ZoneScopedN("write_slice_block.mempcy");
            // 由SliceManager确保opened_write_slice有效，所以不需要再给SliceFile加锁
            uint8_t* dst = slice_block_begin(opened_write_slice, opened_write_slice->writer_index) + block_offset;
            asio::buffer_copy(asio::buffer(dst, slice_block_size - block_offset), buffers);
        }
        
        lock.lock();
        opened_write_slice->is_writing = false;
        opened_write_slice->writer_index++;
        lastest_writer_index = opened_write_slice->writer_index;
        if (opened_write_slice->writer_index >= slice_block_max_count) {
            finish_write_slice(opened_write_slice);
            lastest_writer_index = 0;
            opened_write_slice = nullptr;
            do_prepare_write_slice_async();
            update_read_slices();
        }

        return SliceError::Success;
    }

    SliceReader * add_read_cursor(uint32_t read_cursor) {
        if (read_cursor == 0) {
            return nullptr;
        }
        if (read_cursor >= slice_block_max_count * max_read_slice_count) {
            return nullptr;
        }
        auto lock = acquire_lock();
        std::shared_ptr<SliceReader> reader(new SliceReader);
        reader->read_cursor = read_cursor;
        readers[reader.get()] = reader;
        update_read_slices();

        return reader.get();
    }

    bool remove_slice_cursor(SliceReader *reader) {
        auto lock = acquire_lock();
        return readers.erase(reader);
    }

    template<class MutableBufferSeq>
    std::error_code read_slice_block(SliceReader *in_reader, const MutableBufferSeq &dst_buffers, size_t block_offset) {
        ZoneScopedN("read_slice_block");
        if (block_offset + asio::buffer_size(dst_buffers) > slice_block_size) {
            return SliceError::InvalidArgument;
        }
        auto lock = acquire_lock();
        auto iter = readers.find(in_reader);
        if (iter == readers.end()) {
            return SliceError::InvalidArgument;
        }
        std::shared_ptr<SliceReader> reader = iter->second;
        std::shared_ptr<SliceFile> slice = reader->slice;
        uint32_t block_index = reader->read_index;
        if (!slice) {
            return SliceError::NoSlice;
        }
        if (!slice->is_opened) {
            return SliceError::NotOpened;
        }
        BOOST_ASSERT(block_index < slice->writer_index);
        lock.unlock();

        {
            ZoneScopedN("read_slice_block.memcpy");
            // 为了防止读取时，其他线程写入数据后使slice被close或者free掉，需要对slice加锁
            auto slice_lock = slice->acquire_shared_lock();
            if (!slice->is_opened) {
                return SliceError::NotOpened;
            }
            const uint8_t * src_data = slice_block_begin(slice, block_index) + block_offset;
            asio::buffer_copy(dst_buffers, asio::buffer(src_data, slice_block_size - block_offset));
        }

        lock.lock();
        reader->read_index = block_index + 1;
        if (reader->read_index >= slice->writer_index) {
            update_read_slices();
        }

        return SliceError::Success;
    }

private:
    using Lock = std::unique_lock<std::mutex>;

    Lock acquire_lock() {
        return Lock(mutex);
    }

    Lock acquire_lock_defer() {
        return Lock(mutex, std::defer_lock);
    }

    void do_prepare_write_slice_async() {
        while (!free_slices.empty() && preopen_write_slices.size() < preopen_write_slice_count) {
            auto slice = free_slices.front();
            free_slices.pop_front();
            preopen_write_slices.push_back(slice);
            start_async([this, slice] {
                ZoneScopedN("async_open_write");
                try {
                    logger->debug("slice open write {}", slice->file_name);
                    slice->open(slice_file_size(), true);
                } catch (std::exception &e) {
                    logger->error("do_prepare_write_slice_async error: {}", e.what());
                }
            });
        }
        if (opened_write_slice == nullptr && !preopen_write_slices.empty()) {
            opened_write_slice = preopen_write_slices.front();
            preopen_write_slices.pop_front();
            logger->debug("slice start write {}", opened_write_slice->file_name);
        }
    }

    void finish_write_slice(const std::shared_ptr<SliceFile> &slice) {
        BOOST_ASSERT(slice->is_opened);
        read_slices.push_back(slice);
    }

    bool is_read_cursor_in_slice_range(uint32_t read_cursor, size_t slice_rindex) const {
        if (read_cursor <= lastest_writer_index) {
            return false;
        }
        size_t r = read_cursor - lastest_writer_index;
        return r >= (slice_rindex * slice_block_max_count) + 1 && r <= (slice_rindex + preopen_read_slice_count) * slice_block_max_count;
    }

    void update_read_slices() {
        ZoneScopedN("update_read_slices");
        for (size_t i = 0; read_slices.size() > max_read_slice_count && i < read_slices.size() - max_read_slice_count; ++i) {
            auto &slice = read_slices[i];
            for (auto &p : readers) {
                auto &reader = p.second;
                if (reader->slice == slice) {
                    reader->slice = nullptr;
                }
            }
        }
        for (auto &p : readers) {
            auto &reader = p.second;
            auto read_cursor = reader->read_cursor;
            if (reader->slice && reader->read_index >= reader->slice->writer_index) {
                reader->slice = nullptr;
            }
            if (!reader->slice) {
                BOOST_ASSERT(read_cursor > 0);
                BOOST_ASSERT(lastest_writer_index < slice_block_max_count);
                if (read_cursor <= lastest_writer_index) {
                    reader->slice = opened_write_slice; 
                    reader->read_index = lastest_writer_index - read_cursor;
                    BOOST_ASSERT(reader->read_index < reader->slice->writer_index);
                } else {
                    // read_slices 中每个slice都有 slice_block_max_count 帧数据
                    // r = read_cursor - lastest_writer_index 
                    // (r > 0)
                    // slice_block_max_count * (slice_rindex + 1) = r + read_index
                    size_t read_index = slice_block_max_count - (read_cursor - lastest_writer_index) % slice_block_max_count;
                    if (read_index == slice_block_max_count) {
                        read_index = 0;
                    }
                    size_t slice_rindex = (read_cursor - lastest_writer_index + read_index) / slice_block_max_count - 1;
                    if (slice_rindex < read_slices.size() && slice_rindex < max_read_slice_count) {
                        reader->slice = read_slices[read_slices.size() - 1 - slice_rindex];
                        reader->read_index = read_index;
                        logger->debug("Reader read_cursor={}, slice={}, read_index={}", read_cursor, reader->slice->file_name, read_index);
                        BOOST_ASSERT(reader->read_index < reader->slice->writer_index);
                    }
                }
            }
        }
        size_t i = 0;
        size_t read_slices_old_size = read_slices.size();
        for (auto iter = read_slices.begin(); iter != read_slices.end();) {
            auto &slice = *iter;
            bool should_open = false;
            for (auto &p : readers) {
                auto &reader = p.second;
                if (reader->slice == slice) {
                    should_open = true;
                } else if (is_read_cursor_in_slice_range(reader->read_cursor, read_slices_old_size - i - 1)) {
                    should_open = true;
                }
            }
            bool should_free = !should_open && (read_slices_old_size - i) > max_read_slice_count;

            if (should_open != slice->is_opened) {
                start_async([this, slice, should_open, should_free] {
                    auto lock = acquire_lock_defer();
                    try {
                        if (should_open) {
                            logger->debug("slice open read {}", slice->file_name);
                            ZoneScopedN("async_open_read");
                            slice->open(slice_file_size(), false);
                        } else {
                            ZoneScopedN("async_close");
                            logger->debug("slice close {}", slice->file_name);
                            slice->close();
                        }
                        if (should_free) {
                            logger->debug("slice free {}", slice->file_name);
                            lock.lock();
                            free_slices.push_back(slice);
                        }
                    } catch (std::exception &e) {
                        logger->error("update_read_slices async error: {}", e.what());
                    }
                });
            } else if (should_free) {
                logger->debug("slice free {}", slice->file_name);
                free_slices.push_back(slice);
            }
            if (should_free) {
                iter = read_slices.erase(iter);
            } else {
                ++iter;
            }
            ++i;
        }
    }

    void start_async(std::function<void()> f) {
        asio::post(thread_pool, [f] {
            f();
        });
    }

    uint8_t * slice_block_begin(const std::shared_ptr<SliceFile> &slice, uint32_t block_index) const noexcept {
        return (uint8_t *)slice->data() + slice_first_block_offset + slice_block_size * block_index;
    }

    const size_t slice_file_size() const noexcept {
        return slice_first_block_offset + slice_block_size * slice_block_max_count;
    }

private:
    const size_t max_slice_count;
    const size_t max_read_slice_count;
    const size_t slice_first_block_offset;
    const size_t slice_block_size;
    const size_t slice_block_max_count;

    std::mutex mutex;
    std::deque<std::shared_ptr<SliceFile>> free_slices;
    // 由旧到新，最后一个slice是最近写入的
    std::deque<std::shared_ptr<SliceFile>> read_slices;
    std::deque<std::shared_ptr<SliceFile>> preopen_write_slices;
    std::shared_ptr<SliceFile> opened_write_slice;
    std::unordered_map<SliceReader *, std::shared_ptr<SliceReader>> readers;
    size_t lastest_writer_index = 0;
    asio::thread_pool thread_pool;

    static constexpr size_t preopen_write_slice_count = 3;
    static constexpr size_t preopen_read_slice_count = 3;
};

SliceManager::SliceManager(
    size_t in_max_read_slice_count,
    size_t in_slice_first_block_offset,
    size_t in_slice_block_size,
    size_t in_slice_block_max_count
): p_impl(new Impl(
    in_max_read_slice_count,
    in_slice_first_block_offset,
    in_slice_block_size,
    in_slice_block_max_count
)) {}

SliceManager::~SliceManager() {}

void SliceManager::init(
    const std::string &slice_dir,
    const std::string &slice_file_prefix
) {
    p_impl->init(slice_dir, slice_file_prefix);
}

std::error_code SliceManager::write_slice_block(const boost::asio::const_buffer &buffer, size_t block_offset) {
    return p_impl->write_slice_block(buffer, block_offset);
}


std::error_code SliceManager::write_slice_block(const std::vector<boost::asio::const_buffer> &buffers, size_t block_offset) {
    return p_impl->write_slice_block(buffers, block_offset);
}

std::error_code SliceManager::read_slice_block(SliceReader *in_reader, const boost::asio::mutable_buffer &dst_buffer, size_t block_offset) {
    return p_impl->read_slice_block(in_reader, dst_buffer, block_offset);
}

std::error_code SliceManager::read_slice_block(SliceReader *in_reader, const std::vector<boost::asio::mutable_buffer> &dst_buffers, size_t block_offset) {
    return p_impl->read_slice_block(in_reader, dst_buffers, block_offset);
}


SliceReader * SliceManager::add_read_cursor(uint32_t read_cursor) {
    return p_impl->add_read_cursor(read_cursor);
}

bool SliceManager::remove_slice_cursor(SliceReader *in_reader) {
    return p_impl->remove_slice_cursor(in_reader);
}

} // namespace seeder