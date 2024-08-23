#include "st2110_dummy.h"

#include <boost/asio.hpp>

#include "core/util/buffer.h"
#include "core/util/logger.h"
#include "core/video_format.h"
#include "util/io.h"
#include "util/fs.h"
#include "st2110/st2110_session.h"

namespace asio = boost::asio;

using seeder::core::logger;

namespace seeder {

class St2110DummyFileInput::Impl {
    using Clock = std::chrono::steady_clock;
public:
    ~Impl() {
        if (io_ctx) {
            io_ctx->stop();
        }
    }

    void start() {
        seeder::core::video_format_desc format_desc = seeder::core::video_format_desc::get(config.video_format);
        if (format_desc.is_invalid()) {
            logger->error("St2110DummyFileInput invalid video_format: {}", config.video_format);
            throw std::runtime_error("video_format");
        }
        auto framesize = get_st2110_20_frame_size(format_desc);
        auto ec = seeder::util::read_file(config.file, buffer);
        if (ec) {
            logger->error("St2110DummyFileInput read_file {} failed: {}", config.file, ec.message());
            throw std::runtime_error("read_file");
        }
        if (buffer->get_size() != framesize) {
            logger->error("St2110DummyFileInput filesize mismsatch {} {}", framesize, buffer->get_size());
            throw std::runtime_error("framesize");
        }
        ns_per_frame = format_desc.ns_per_frame();
        start_time = Clock::now();
        join_handle = seeder::async_run_with_new_io_context();
        io_ctx = join_handle.get_io_ctx();
        start_timer();
    }

    void start_timer() {
        timer.reset(new asio::steady_timer(*io_ctx));
        async_wait_next_frame();
    }

    void output_one_frame() {
        source->push_video_frame(*buffer);
    }

    void async_wait_next_frame() {
        timer->expires_at(next_frame_time_from_now());
        timer->async_wait([this] (const boost::system::error_code &ec) {
            if (ec) {
                return;
            }
            output_one_frame();
            async_wait_next_frame();
        });
    }

    Clock::time_point next_frame_time_from_now() {
        auto now = Clock::now();
        auto ns_from_start = std::chrono::duration_cast<std::chrono::nanoseconds>((now - start_time)).count();
        std::chrono::nanoseconds::rep next_frame_num = std::ceil(ns_from_start / ns_per_frame);
        std::chrono::nanoseconds::rep next_frame_ns = next_frame_num * ns_per_frame;
        Clock::time_point next_frame_time = start_time + std::chrono::nanoseconds(next_frame_ns);
        return next_frame_time;
    }

    seeder::config::St2110DummyFileInputConfig config;
    std::shared_ptr<St2110BaseSource> source;
    std::shared_ptr<seeder::core::AbstractBuffer> buffer;
    AsioJoinHandle join_handle;
    std::unique_ptr<asio::steady_timer> timer;
    Clock::time_point start_time;
    double ns_per_frame = 0;
    std::shared_ptr<boost::asio::io_context> io_ctx;
};

St2110DummyFileInput::St2110DummyFileInput(const seeder::config::St2110DummyFileInputConfig &in_config):
    p_impl(new Impl) {

    p_impl->config = in_config;
}

St2110DummyFileInput::~St2110DummyFileInput() {}

void St2110DummyFileInput::set_source(const std::shared_ptr<St2110BaseSource> &in_source) {
    p_impl->source = in_source;
}

void St2110DummyFileInput::start() {
    p_impl->start();
}

} // namespace seeder