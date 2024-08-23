#pragma once

#include <functional>
#include <memory>
#include <thread>

#include <boost/asio/io_context.hpp>


namespace seeder {


class JoinHandle {
public:
    explicit JoinHandle() {}

    explicit JoinHandle(std::thread &&in_thread): thread(std::move(in_thread)) {}

    JoinHandle(const JoinHandle &) = delete;
    JoinHandle & operator=(const JoinHandle &) = delete;

    JoinHandle(JoinHandle &&other): thread(std::move(other.thread)) {}

    JoinHandle & operator=(JoinHandle &&other) {
        release();
        thread = std::move(other.thread);
        return *this;
    }

    ~JoinHandle() {
        release();
    }

    operator bool() const {
        return thread.get_id() != std::thread::id();
    }

    std::thread::id get_id() const {
        return thread.get_id();
    }

private:
    void release() {
        if (thread.get_id() != std::thread::id() && thread.joinable()) {
            thread.join();
        }
        thread = std::thread();
    }

private:
    std::thread thread;
};


class AsioJoinHandle {
public:
    explicit AsioJoinHandle() {}
    explicit AsioJoinHandle(
        JoinHandle &&in_join_handle, 
        const std::shared_ptr<boost::asio::io_context> &in_io_ctx
    ):
        join_handle(std::move(in_join_handle)), 
        io_ctx(in_io_ctx)
    {}

    AsioJoinHandle(AsioJoinHandle &&) = default;

    AsioJoinHandle & operator=(AsioJoinHandle &&other) {
        stop();
        join_handle = std::move(other.join_handle);
        io_ctx = std::move(other.io_ctx);
        return *this;
    }

    ~AsioJoinHandle() {
        stop();
    }

    const std::shared_ptr<boost::asio::io_context> & get_io_ctx() const {
        return io_ctx;
    }

    operator bool() const {
        return static_cast<bool>(join_handle);
    }

    void stop() {
        if (io_ctx) {
            io_ctx->stop();
        }
    }

    std::thread::id get_thread_id() {
        return join_handle.get_id();
    }

private:
    JoinHandle join_handle;
    std::shared_ptr<boost::asio::io_context> io_ctx;
};


// 在新线程中执行直到io_ctx.stop()被调用
[[nodiscard]]
AsioJoinHandle async_run_with_new_io_context();

void async_run_with_new_io_context(
    AsioJoinHandle &out_join_handle,
    std::function<void()> startup
);

[[nodiscard]]
JoinHandle async_run_with_io_context(
    const std::shared_ptr<boost::asio::io_context> &io_ctx
);


void async_run_with_io_context_detach(
    const std::shared_ptr<boost::asio::io_context> &io_ctx
);

} // namespace seeder