#pragma once

#include <memory>
#include <functional>

#include <core/video_format.h>
#include <core/util/buffer.h>

namespace seeder {

namespace config {

struct ST2110Config;
struct ST2110InputConfig;
struct ST2110OutputConfig;
struct ST2110AudioConfig;

} // namespace seeder::config

class St2110BaseOutput;
class St2110BaseSource;

class St2110TxSession {
    friend class St2110SessionManager;
public:
    explicit St2110TxSession();
    ~St2110TxSession();

    // 可能会抛出异常
    void start();

    void stop();

    std::shared_ptr<seeder::St2110BaseOutput> get_output();

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};


struct St2110RxVideoFrameMeta {
    bool is_second_field = false;
};


class St2110RxSession {
    friend class St2110SessionManager;
public:
    explicit St2110RxSession();
    ~St2110RxSession();

    // 可能会抛出异常
    void start();

    void stop();

    std::shared_ptr<seeder::St2110BaseSource> get_source();

    using VideoBufferAllocator = std::function<std::shared_ptr<seeder::core::AbstractBuffer>(St2110BaseSource &, std::size_t)>;
    void set_video_buffer_allocator(VideoBufferAllocator in_allocator);

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};


class St2110SessionManager {
public:
    explicit St2110SessionManager();
    ~St2110SessionManager();

    // 可能阻塞，并且会将线程绑定到某个核上，最好在新线程中执行
    void init(const seeder::config::ST2110Config &config);

    // 销毁所有session并关闭
    void close();

    void set_output_factory(std::function<std::shared_ptr<St2110BaseOutput>(const seeder::config::ST2110OutputConfig &)> in_factory);

    std::shared_ptr<St2110TxSession> create_tx_session(
        const seeder::config::ST2110OutputConfig &output_config);

    void remove_tx_session(const std::string &id);

    void set_source_factory(std::function<std::shared_ptr<St2110BaseSource>(const seeder::config::ST2110InputConfig &)> in_factory);

    // 可能抛出异常
    std::shared_ptr<St2110RxSession> create_rx_session(
        const seeder::config::ST2110InputConfig &input_config);

    std::shared_ptr<St2110RxSession> get_rx_session(const std::string &id) const;

    void remove_rx_session(const std::string &id);

    void set_rx_video_buffer_allocator(St2110RxSession::VideoBufferAllocator in_allocator);

    void rx_video_session_change_ip(const seeder::config::ST2110InputConfig &input_config);

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

void setup_st2110_audio_format(
    seeder::core::video_format_desc &format_desc,
    const seeder::config::ST2110AudioConfig &audio_config
);

std::size_t get_st2110_20_frame_size(const seeder::core::video_format_desc &format_desc);

} // namespace seeder