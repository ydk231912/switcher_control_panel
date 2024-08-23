#include "io.h"

#include <thread>

#include <boost/asio.hpp>


namespace asio = boost::asio;


namespace {


std::thread async_run(const std::shared_ptr<boost::asio::io_context> &io_ctx) {
    return std::thread([io_ctx] {
        auto guard = asio::make_work_guard(*io_ctx);
        io_ctx->run();
    });
}


} // namespace


namespace seeder {


AsioJoinHandle async_run_with_new_io_context() {
    std::shared_ptr<boost::asio::io_context> io_ctx(new asio::io_context);
    return AsioJoinHandle(
        JoinHandle(async_run(io_ctx)),
        io_ctx
    );
}

void async_run_with_new_io_context(
    AsioJoinHandle &out_join_handle,
    std::function<void()> startup
) {
    std::shared_ptr<boost::asio::io_context> io_ctx(new asio::io_context);
    out_join_handle = AsioJoinHandle(
        JoinHandle(async_run(io_ctx)),
        io_ctx
    );
    asio::post(*io_ctx, startup);
}

JoinHandle async_run_with_io_context(
    const std::shared_ptr<boost::asio::io_context> &io_ctx
) {
    return JoinHandle(async_run(io_ctx));
}

void async_run_with_io_context_detach(
    const std::shared_ptr<boost::asio::io_context> &io_ctx
) {
    async_run(io_ctx).detach();
}


} // namespace seeder