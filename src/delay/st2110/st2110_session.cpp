#include "st2110_session.h"

#include <deque>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <list>
#include <cstdio>

#include <arpa/inet.h>
#include <mtl/mtl_api.h>
#include <mtl/st20_api.h>
#include <mtl/st30_api.h>
#include <pthread.h>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <core/util/buffer.h>
#include "core/util/logger.h"
#include "core/util/thread.h"
#include "core/video_format.h"

#include "st2110_output.h"
#include "st2110_source.h"
#include "st2110_config.h"

#include "util/stat_recorder.h"


#ifndef NS_PER_S
#define NS_PER_S (1000000000)
#endif

#define ST_APP_MAX_LCORES (32)


namespace {

std::shared_ptr<spdlog::logger> logger;

struct st_video_fmt_desc {
    enum seeder::core::video_fmt core_fmt;
    enum st_fps fps;
    bool interlaced;
};

const std::vector<st_video_fmt_desc> st_video_fmt_descs = {
    {
        seeder::core::video_fmt::x2160p5000,
        ST_FPS_P50,
        false
    },
    {
        seeder::core::video_fmt::x2160p6000,
        ST_FPS_P60,
        false
    },
    {
        seeder::core::video_fmt::x1080p5000,
        ST_FPS_P50,
        false
    },
    {
        seeder::core::video_fmt::x1080p6000,
        ST_FPS_P60,
        false
    },
    {
        seeder::core::video_fmt::x1080i5000,
        ST_FPS_P50,
        true
    },
};

template<class T>
struct ConfigEnumDesc {
    std::string name;
    T value;

    using ValueType = T;
};

template<class C, class S>
std::optional<typename C::value_type::ValueType> parse_value_from_desc_name(const C &collection, S &&name) noexcept {
    for (auto &d : collection) {
        if (d.name == name) {
            return d.value;
        }
    }
    return std::nullopt;
}

template<class C, class S, class M>
typename C::value_type::ValueType parse_value_from_desc_name_or_throw(const C &collection, S &&name, M &&error_message) {
    auto opt = parse_value_from_desc_name(collection, name);
    if (opt) {
        return *opt;
    } else {
        throw std::invalid_argument((
            boost::format("parse_value_from_desc_name_or_throw %s: %s") % std::forward<M>(error_message) % std::forward<S>(name)
        ).str());
    }
}

const std::vector<ConfigEnumDesc<enum st30_fmt>> st30_fmt_desc_list {
    {"PCM8", ST30_FMT_PCM8},
    {"PCM16", ST30_FMT_PCM16},
    {"PCM24", ST30_FMT_PCM24},
    {"AM824", ST31_FMT_AM824}
};

const std::vector<ConfigEnumDesc<int>> audio_channel_desc_list {
    {"M", 1},
    {"ST", 2}
};

const std::vector<ConfigEnumDesc<enum st30_sampling>> st30_sampling_desc_list {
    {"48kHz", ST30_SAMPLING_48K},
    {"96kHz", ST30_SAMPLING_96K},
    {"44.1kHz", ST31_SAMPLING_44K},
};

const std::vector<ConfigEnumDesc<enum st30_ptime>> st30_ptime_desc_list {
    {"1", ST30_PTIME_1MS},
    {"0.25", ST30_PTIME_250US}
};

std::string parse_audio_ptime(const seeder::config::ST2110AudioConfig &audio) {
    if (audio.audio_ptime.empty()) {
        return "1";
    } else {
        return audio.audio_ptime;
    }
}

const st_video_fmt_desc * get_video_fmt_from_core_fmt(seeder::core::video_fmt core_fmt) {
    for (auto &desc : st_video_fmt_descs) {
        if (desc.core_fmt == core_fmt) {
            return &desc;
        }
    }
    return nullptr;
}

std::size_t calc_st30_frame_size(st30_fmt fmt, st30_ptime ptime, st30_sampling sampling, int channel) {
    int pkt_size = st30_get_packet_size(fmt, ptime, sampling, channel);
    double pkt_time = st30_get_packet_time(ptime);
    constexpr int NS_PER_MS = (1000 * 1000);
    int pkt_per_frame = NS_PER_MS / pkt_time;
    return pkt_per_frame * pkt_size;
}

ssize_t console_log_write(void *c, const char *buf, size_t size) {
    ssize_t ret;
	/* write on stderr */
	ret = fwrite(buf, 1, size, stderr);
	fflush(stderr);
    return ret;
}

void init_st2110_log() {
    cookie_io_functions_t log_func {};
    log_func.write = console_log_write;
    FILE *log_stream = fopencookie(NULL, "w+", log_func);
    if (log_stream == NULL) {
        logger->error("fopencookie failed");
        return;
    }
    if (mtl_openlog_stream(log_stream)) {
        logger->error("mtl_openlog_stream failed");
    }
}

class StMainContext {
public:
    explicit StMainContext() {
        lcore_list.resize(ST_APP_MAX_LCORES);
    }

    void init(const seeder::config::ST2110Config &config) {
        init_st2110_log();

        memset(&params, 0, sizeof(params));
        init_params(config);
        handle = mtl_init(&params);
        if (!handle) {
            logger->error("mtl_init failed");
            throw std::runtime_error("mtl_init");
        }
    }

    void start() {
        int ret = mtl_start(handle);
        if (ret) {
            logger->error("mtl_start failed: {}", ret);
            throw std::runtime_error("mtl_start");
        }
        is_started = true;
    }

    void init_params(const seeder::config::ST2110Config &config) {
        devices = config.devices;
        st2110_config = config;

        params.pmd[MTL_PORT_P] = MTL_PMD_DPDK_USER;
        params.pmd[MTL_PORT_R] = MTL_PMD_DPDK_USER;
        params.xdp_info[MTL_PORT_P].start_queue = 1;
        params.xdp_info[MTL_PORT_R].start_queue = 1;
        params.flags |= MTL_FLAG_BIND_NUMA; /* default bind to numa */
        params.flags |= MTL_FLAG_TX_VIDEO_MIGRATE;
        params.flags |= MTL_FLAG_RX_VIDEO_MIGRATE;
        params.flags |= MTL_FLAG_RX_SEPARATE_VIDEO_LCORE;
        params.priv = this;
        params.stat_dump_cb_fn = app_stat;
        params.dump_period_s = 10;
        params.log_level = MTL_LOG_LEVEL_INFO; // ST_LOG_LEVEL_INFO;

        auto &devices = config.devices;
        params.num_ports = devices.size();
        int max_tx_queue_count = 32 * 2 * 2;
        int max_rx_queue_count = 8 * 2 * 2;
#if MTL_VERSION_MAJOR < 23
        params.rx_sessions_cnt_max = max_tx_queue_count;
        params.tx_sessions_cnt_max = max_rx_queue_count;
#endif
        for (int i = 0; i < devices.size(); i++) {
            strncpy(params.port[i], devices[i].device.data(), sizeof(params.port[i]));
            inet_pton(AF_INET, devices[i].sip.data(), params.sip_addr[i]);
            params.pmd[i] = mtl_pmd_by_port_name(params.port[i]);
#if MTL_VERSION_MAJOR >= 23
            params.tx_queues_cnt[i] = max_tx_queue_count;
            params.rx_queues_cnt[i] = max_rx_queue_count;
#else
            params.xdp_info[i].queue_count = max_tx_queue_count + max_rx_queue_count;
#endif
        }

        if (config.ptp.enable) {
            params.flags |= MTL_FLAG_PTP_ENABLE;
        } else {
            params.ptp_get_time_fn = app_ptp_from_tai_time;
        }

        if (st2110_config.sch_quota) {
            params.data_quota_mbs_per_sch = st2110_config.sch_quota * st20_1080p59_yuv422_10bit_bandwidth_mps();
            logger->debug("sch_quota={} data_quota_mbs_per_sch={}", st2110_config.sch_quota, params.data_quota_mbs_per_sch);
        }
    }

    std::string get_port(int device_id) {
        for (auto &d : devices) {
            if (device_id == d.id) {
                return d.device;
            }
        }
        return "";
    }

    static uint64_t app_ptp_from_tai_time(void *priv) {
        // Impl *p_impl = (Impl *)priv;
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        return ((uint64_t)spec.tv_sec * NS_PER_S) + spec.tv_nsec;
    }

    static void app_stat(void *priv) {}

    void close() {
        mtl_stop(handle);
    }

    void bind_to_lcore(unsigned int lcore) {
        mtl_bind_to_lcore(handle, pthread_self(), lcore);
    }

    std::optional<int> acquire_lcore(int sch_idx) {
        unsigned int video_lcore;
        if (sch_idx >= lcore_list.size()) {
            return std::nullopt;
        }
        if (lcore_list[sch_idx].has_value()) {
            return lcore_list[sch_idx];
        }
        int ret = mtl_get_lcore(handle, &video_lcore);
        if (ret < 0) {
            logger->error("mtl_get_lcore failed: {}", ret);
            return std::nullopt;
        }
        lcore_list[sch_idx] = video_lcore;
        return video_lcore;
    }

    struct mtl_init_params params;
    mtl_handle handle = 0;
    std::vector<std::optional<int>> lcore_list;
    std::vector<seeder::config::ST2110DeviceConfig> devices;
    seeder::config::ST2110Config st2110_config;
    bool is_started = false;
};

template<class T>
class BlockingQueue {
public:
    bool pop(T &out) {
        std::unique_lock<std::mutex> lock(mutex);
        while (!is_closed) {
            if (queue.empty()) {
                condition_variable.wait(lock);
                continue;
            }
            out = queue.front();
            queue.pop_front();
            return true;
        }
        return false;
    }

    bool push(const T &item) {
        std::unique_lock<std::mutex> lock(mutex);
        if (is_closed) {
            return false;
        }
        queue.push_back(item);
        condition_variable.notify_one();
        return true;
    }

    void close() {
        std::unique_lock<std::mutex> lock(mutex);
        is_closed = true;
        condition_variable.notify_all();
    }

    void reset() {
        std::unique_lock<std::mutex> lock(mutex);
        is_closed = false;
        queue.clear();   
    }

private:
    std::mutex mutex;
    std::condition_variable condition_variable;
    bool is_closed = false;
    std::deque<T> queue;
};

class RxFrameWrapBuffer : public seeder::core::AbstractBuffer {
public:
    explicit RxFrameWrapBuffer(void *in_data, std::size_t in_size): data(in_data), size(in_size) {}
    explicit RxFrameWrapBuffer(
        void *in_data, std::size_t in_size, const std::any &in_meta
    ): 
        data(in_data), size(in_size), meta(in_meta)
    {}

    void * get_data() override {
        return data;
    }

    std::size_t get_size() const override {
        return size;
    }

    std::any get_meta() const override {
        return meta;
    }

private:
    void *data = nullptr;
    std::size_t size = 0;
    std::any meta;
};

} // namespace


namespace seeder {

class St2110RxVideoSession : public std::enable_shared_from_this<St2110RxVideoSession> {
public:
    struct FramebufferInfo {
        void *data = nullptr;
        struct st20_rx_frame_meta meta;
    };

    // may throw exception
    void init(const seeder::config::ST2110InputConfig &in_input_config) {
        auto &input_config = in_input_config;
        auto main_ctx = weak_main_ctx.lock();

        memset(&ops, 0, sizeof(ops));
        short_name.append("rv_").append(input_config.video.ip);
        ops.name = short_name.c_str();
        ops.priv = this;
        
        assign_ip(in_input_config);
        ops.pacing = ST21_PACING_NARROW;
        ops.packing = ST20_PACKING_BPM;
        ops.type = ST20_TYPE_FRAME_LEVEL;
        ops.flags = ST20_RX_FLAG_DMA_OFFLOAD;
        auto &format_desc = seeder::core::video_format_desc::get(input_config.video.video_format);
        const st_video_fmt_desc *st_fmt_desc = get_video_fmt_from_core_fmt(format_desc.format);
        ops.width = format_desc.width;
        ops.height = format_desc.height;
        ops.fps = st_fmt_desc->fps;
        ops.fmt = ST20_FMT_YUV_422_10BIT;
        ops.payload_type = input_config.video.payload_type;
        ops.interlaced = st_fmt_desc->interlaced;
        ops.notify_frame_ready = rx_video_frame_ready;
        ops.framebuff_cnt = framebuff_cnt;

        frame_bytes_size = st20_frame_size(ops.fmt, ops.width, ops.height);
    }

    void assign_ip(const seeder::config::ST2110InputConfig &input_config) {
        auto main_ctx = weak_main_ctx.lock();

        ops.num_port = 1;
        strncpy(ops.port[MTL_PORT_P], main_ctx->get_port(input_config.device_id).c_str(), sizeof(ops.port[MTL_PORT_P]));
        inet_pton(AF_INET, input_config.video.ip.data(), ops.sip_addr[MTL_PORT_P]);
        ops.udp_port[MTL_PORT_P] = input_config.video.port;
        if (input_config.video.redundant.has_value() && input_config.video.redundant->enable) {
            ops.num_port = 2;
            strncpy(ops.port[MTL_PORT_R], main_ctx->get_port(input_config.video.redundant->device_id).c_str(), sizeof(ops.port[MTL_PORT_R]));
            inet_pton(AF_INET, input_config.video.redundant->ip.c_str(), ops.sip_addr[MTL_PORT_R]);
            ops.udp_port[MTL_PORT_R] = input_config.video.redundant->port;
        }
    }

    void init_alloc_framebuffers() {
        if (!rx_frame_allocator) {
            return;
        }
        auto main_ctx = weak_main_ctx.lock();
        size_t page_size = mtl_page_size(main_ctx->handle);
        ext_frame_align_size = MTL_ALIGN(frame_bytes_size, page_size);
        for (std::size_t i = 0; i < framebuff_cnt; ++i) {
            auto &ext_frame_info = ext_frame_infos.emplace_back();
            ext_frame_info.buf_iova = MTL_BAD_IOVA;
            std::shared_ptr<seeder::core::AbstractBuffer> ext_frame_buffer = rx_frame_allocator(*source, ext_frame_align_size);
            void *fb_src = (void*)MTL_ALIGN(((size_t)ext_frame_buffer->get_data()), page_size);
            // mtl_dma_map 的size必须是page_size的整数倍
            mtl_iova_t fb_src_iova = mtl_dma_map(main_ctx->handle, fb_src, ext_frame_align_size);
            if (fb_src_iova == MTL_BAD_IOVA) {
                logger->error("rx_video init mtl_dma_map failed");
                throw std::runtime_error("mtl_dma_map");
            }
            ext_frame_info.buf_addr = fb_src;
            ext_frame_info.buf_iova = fb_src_iova;
            ext_frame_info.buf_len = frame_bytes_size;
            ext_frame_info.opaque = NULL;

            ext_frame_buffers.push_back(ext_frame_buffer);
        }
        ops.ext_frames = &ext_frame_infos[0];
    }

    // may throw exception
    void start() {
        if (is_started) {
            return;
        }
        logger->info("St2110RxVideoSession {} start", short_name);
        auto main_ctx = weak_main_ctx.lock();
        init_alloc_framebuffers();
        frame_queue.reset();
        handle = st20_rx_create(main_ctx->handle, &ops);
        if (!handle) {
            throw std::runtime_error("st20_rx_create");
        }
        int handle_sch_idx = st20_rx_get_sch_idx(handle);
        if (handle_sch_idx >= 0) {
            lcore = main_ctx->acquire_lcore(handle_sch_idx);
        }
        auto shared_this = shared_from_this();
        is_started = true;
        thread = std::thread([shared_this] {
            shared_this->run();
        });
    }

    void run() {
        seeder::core::util::set_thread_name(short_name);

        if (lcore) {
            auto main_ctx = weak_main_ctx.lock();
            main_ctx->bind_to_lcore(lcore.value());
        }
        FramebufferInfo frame;
        while (frame_queue.pop(frame)) {
            St2110RxVideoFrameMeta meta;
            meta.is_second_field = frame.meta.second_field;
            RxFrameWrapBuffer buffer(frame.data, frame_bytes_size, meta);
            source->push_video_frame(buffer);
            st20_rx_put_framebuff(handle, frame.data);
        }
        logger->info("St2110RxVideoSession {} run finish", short_name);
    }

    void close() {
        if (!is_started) {
            return;
        }
        logger->info("St2110RxVideoSession {} close", short_name);

        is_started = false;
        frame_queue.close();
        thread.join();
        int ret = 0;

        auto main_ctx = weak_main_ctx.lock();
        for (auto &ext_frame : ext_frame_infos) {
            if (ext_frame.buf_iova != MTL_BAD_IOVA) {
                ret = mtl_dma_unmap(main_ctx->handle, ext_frame.buf_addr, ext_frame.buf_iova, ext_frame_align_size);
                if (ret < 0) {
                    logger->error("rx_video st_app_rx_video_session_uinit mtl_dma_unmap failed");
                }
            }
            ext_frame_infos.clear();
        }
        if (handle) {
            ret = st20_rx_free(handle);
            if (ret < 0) {
                logger->error("st20_rx_free failed: {}", ret);
            }
            handle = 0;
        }
    }

    static int rx_video_frame_ready(void *priv, void *frame, struct st20_rx_frame_meta *meta) {
        St2110RxVideoSession *s = (St2110RxVideoSession *)priv;
        FramebufferInfo f;
        f.data = frame;
        f.meta = *meta;
        s->frame_queue.push(f);
        return 0;
    }

    bool is_started = false;
    std::size_t framebuff_cnt = 3;
    std::weak_ptr<StMainContext> weak_main_ctx;
    st20_rx_handle handle = 0;
    std::optional<int> lcore;
    struct st20_rx_ops ops;
    std::string short_name;
    std::size_t frame_bytes_size = 0;
    std::size_t ext_frame_align_size = 0;
    std::vector<st20_ext_frame> ext_frame_infos;
    std::vector<std::shared_ptr<seeder::core::AbstractBuffer>> ext_frame_buffers;
    std::shared_ptr<St2110BaseSource> source;
    std::thread thread;
    BlockingQueue<FramebufferInfo> frame_queue;
    St2110RxSession::VideoBufferAllocator rx_frame_allocator;
};

class St2110RxAudioSession : public std::enable_shared_from_this<St2110RxAudioSession> {
public:
    struct FramebufferInfo {
        void *data = nullptr;
        struct st30_rx_frame_meta meta;
    };

    void init(const seeder::config::ST2110InputConfig &in_input_config) {
        auto &input_config = in_input_config;
        auto main_ctx = weak_main_ctx.lock();

        memset(&ops, 0, sizeof(ops));
        short_name.append("ra_").append(input_config.audio.ip);
        ops.name = short_name.c_str();
        ops.priv = this;
        ops.num_port = 1;
        strncpy(ops.port[MTL_PORT_P], main_ctx->get_port(input_config.device_id).c_str(), sizeof(ops.port[MTL_PORT_P]));
        inet_pton(AF_INET, input_config.audio.ip.data(), ops.sip_addr[MTL_PORT_P]);
        ops.udp_port[MTL_PORT_P] = input_config.audio.port;
        if (input_config.audio.redundant.has_value() && input_config.audio.redundant->enable) {
            ops.num_port = 2;
            strncpy(ops.port[MTL_PORT_R], main_ctx->get_port(input_config.audio.redundant->device_id).c_str(), sizeof(ops.port[MTL_PORT_R]));
            inet_pton(AF_INET, input_config.audio.redundant->ip.data(), ops.sip_addr[MTL_PORT_R]);
            ops.udp_port[MTL_PORT_R] = input_config.audio.redundant->port;
        }

        ops.type = ST30_TYPE_FRAME_LEVEL;
        ops.fmt = parse_value_from_desc_name_or_throw(
            st30_fmt_desc_list, 
            input_config.audio.audio_format, 
            "audio_format");
        ops.payload_type = input_config.audio.payload_type;
        ops.channel = parse_value_from_desc_name_or_throw(
            audio_channel_desc_list,
            input_config.audio.audio_channel,
            "audio_channel"
        );
        ops.sampling = parse_value_from_desc_name_or_throw(
            st30_sampling_desc_list,
            input_config.audio.audio_sampling,
            "audio_sampling"
        );
        ops.ptime = parse_value_from_desc_name_or_throw(
            st30_ptime_desc_list,
            parse_audio_ptime(input_config.audio),
            "audio_ptime"
        );
        // ops.sample_size = st30_get_sample_size(ops.fmt);
        // ops.sample_num = st30_get_sample_num(ops.ptime, ops.sampling);
        ops.framebuff_size = calc_st30_frame_size(ops.fmt, ops.ptime, ops.sampling, ops.channel);
        ops.framebuff_cnt = framebuff_cnt;

        ops.notify_frame_ready = frame_ready_static;

        frame_bytes_size = ops.framebuff_size;
    }

    void start() {
        logger->info("St2110RxAudioSession {} start", short_name);
        auto main_ctx = weak_main_ctx.lock();
        frame_queue.reset();
        handle = st30_rx_create(main_ctx->handle, &ops);
        if (!handle) {
            throw std::runtime_error("st30_rx_create");
        }
        auto shared_this = shared_from_this();
        is_started = true;
        thread = std::thread([shared_this] {
            shared_this->run();
        });
    }

    void run() {
        seeder::core::util::set_thread_name(short_name);

        FramebufferInfo frame;
        while (frame_queue.pop(frame)) {
            RxFrameWrapBuffer buffer(frame.data, frame_bytes_size);
            source->push_audio_frame(buffer);
            st30_rx_put_framebuff(handle, frame.data);
        }
        logger->info("St2110RxAudioSession {} run finish", short_name);
    }

    void close() {
        if (!is_started) {
            return;
        }
        logger->info("St2110RxAudioSession {} start", short_name);

        is_started = false;
        frame_queue.close();
        thread.join();

        int ret = 0;
        if (handle) {
            ret = st30_rx_free(handle);
            if (ret < 0) {
                logger->error("st30_rx_free failed: {}", ret);
            }
            handle = 0;
        }
    }

    static int frame_ready_static(void* priv, void* frame, struct st30_rx_frame_meta *meta) {
        St2110RxAudioSession *s = (St2110RxAudioSession *)priv;
        FramebufferInfo f;
        f.data = frame;
        f.meta = *meta;
        s->frame_queue.push(f);
        return 0;
    }

    bool is_started = false;
    struct st30_rx_ops ops;
    std::weak_ptr<StMainContext> weak_main_ctx;
    st30_rx_handle handle = 0;
    std::string short_name;
    std::size_t framebuff_cnt = 20;
    std::size_t frame_bytes_size = 0;
    std::shared_ptr<St2110BaseSource> source;
    std::thread thread;
    BlockingQueue<FramebufferInfo> frame_queue;
};

class St2110TxVideoSession : public std::enable_shared_from_this<St2110TxVideoSession> {
public:
    struct FramebufferInfo {
        void *data = nullptr;
        std::optional<struct st20_tx_frame_meta> meta;
    };

    void init(const seeder::config::ST2110OutputConfig &in_output_config) {
        output_config = in_output_config;
        auto main_ctx = weak_main_ctx.lock();

        memset(&ops, 0, sizeof(ops));
        short_name.append("tv_").append(output_config.video.ip);
        ops.name = short_name.c_str();
        ops.priv = this;
        ops.num_port = 1;
        strncpy(ops.port[MTL_PORT_P], main_ctx->get_port(output_config.device_id).c_str(), MTL_PORT_MAX_LEN);
        inet_pton(AF_INET, output_config.video.ip.c_str(), ops.dip_addr[MTL_PORT_P]);
        ops.udp_port[MTL_PORT_P] = output_config.video.port;
        if (output_config.video.redundant.has_value() && output_config.video.redundant->enable) {
            ops.num_port = 2;
            strncpy(ops.port[MTL_PORT_R], main_ctx->get_port(output_config.video.redundant->device_id).c_str(), MTL_PORT_MAX_LEN);
            inet_pton(AF_INET, output_config.video.redundant->ip.c_str(), ops.dip_addr[MTL_PORT_R]);
            ops.udp_port[MTL_PORT_R] = output_config.video.redundant->port;
        }
        ops.pacing = ST21_PACING_NARROW;
        ops.packing = ST20_PACKING_BPM;
        ops.type = ST20_TYPE_FRAME_LEVEL;
        auto &format_desc = seeder::core::video_format_desc::get(output_config.video.video_format);
        const st_video_fmt_desc *st_fmt_desc = get_video_fmt_from_core_fmt(format_desc.format);
        ops.width = format_desc.width;
        ops.height = format_desc.height;
        ops.fps = st_fmt_desc->fps;
        ops.fmt = ST20_FMT_YUV_422_10BIT;
        ops.payload_type = output_config.video.payload_type;
        ops.interlaced = st_fmt_desc->interlaced;
        ops.framebuff_cnt = framebuff_cnt;

        ops.get_next_frame = get_next_frame_static;
        ops.notify_frame_done = frame_done_static;

        frame_list.resize(framebuff_cnt);
    }

    void start() {
        logger->info("St2110TxVideoSession {} start", short_name);
        auto main_ctx = weak_main_ctx.lock();

        handle = st20_tx_create(main_ctx->handle, &ops);
        if (!handle) {
            throw std::runtime_error("st20_tx_create");
        }
        frame_bytes_size = st20_tx_get_framebuffer_size(handle);
        int handle_sch_idx = st20_tx_get_sch_idx(handle);
        if (handle_sch_idx >= 0) {
            lcore = main_ctx->acquire_lcore(handle_sch_idx);
        }
        for (std::size_t i = 0; i < frame_list.size(); ++i) {
            auto &frame_info = frame_list[i];
            frame_info.data = st20_tx_get_framebuffer(handle, i);
            writable_frame_queue.push_back(i);
        }
        auto shared_this = shared_from_this();
        is_started = true;
        is_closed = false;
        thread = std::thread([shared_this] {
            shared_this->run();
        });
    }

    void run() {
        seeder::core::util::set_thread_name(short_name);

        if (lcore) {
            auto main_ctx = weak_main_ctx.lock();
            main_ctx->bind_to_lcore(lcore.value());
        }
        {
            std::unique_lock<std::mutex> lock(mutex);
            auto output = this->output;
            while (!is_closed) {
                std::size_t writable_frame_index;
                if (writable_frame_queue.empty()) {
                    condition_variable.wait(lock);
                    continue;
                }
                writable_frame_index = writable_frame_queue.front();
                writable_frame_queue.pop_front();
                lock.unlock();

                auto &frame = frame_list[writable_frame_index];
                if (!output->wait_get_video_frame(frame.data)) {
                    break;
                }

                lock.lock();
                readable_frame_queue.push_back(writable_frame_index);
            }
        }
        logger->info("St2110TxVideoSession {} run finish", short_name);
    }

    void close() {
        if (!is_started) {
            return;
        }
        logger->info("St2110TxVideoSession {} close", short_name);

        is_started = false;
        {
            std::unique_lock<std::mutex> lock(mutex);
            is_closed = true;
            condition_variable.notify_all();
        }
        thread.join();

        int ret = st20_tx_free(handle);
        if (ret < 0) {
            logger->error("st20_tx_free failed: {}", ret);
        }
        handle = 0;
    }

    static int get_next_frame_static(void *priv, uint16_t *next_frame_idx, struct st20_tx_frame_meta *meta) {
        St2110TxVideoSession *s = (St2110TxVideoSession *) priv;
        if (s->require_next_frame(next_frame_idx)) {
            return 0;
        } else {
            return -EIO;
        }
    }

    bool require_next_frame(uint16_t *next_frame_idx) {
        std::unique_lock<std::mutex> lock(mutex);
        if (readable_frame_queue.empty()) {
            return false;
        }
        std::size_t frame_index = readable_frame_queue.front();
        readable_frame_queue.pop_front();
        *next_frame_idx = frame_index;
        return true;
    }

    static int frame_done_static(void *priv, uint16_t frame_idx, struct st20_tx_frame_meta *meta) {
        St2110TxVideoSession *s = (St2110TxVideoSession *) priv;
        s->frame_done(frame_idx);
        return 0;
    }

    void frame_done(uint16_t frame_idx) {
        std::unique_lock<std::mutex> lock(mutex);
        writable_frame_queue.push_back(frame_idx);
        condition_variable.notify_one();
    }

    seeder::config::ST2110OutputConfig output_config;
    bool is_started = false;
    std::size_t framebuff_cnt = 3;
    std::size_t frame_bytes_size = 0;
    std::weak_ptr<StMainContext> weak_main_ctx;
    st20_tx_handle handle;
    std::optional<int> lcore;
    struct st20_tx_ops ops;
    std::string short_name;
    std::shared_ptr<St2110BaseOutput> output;
    std::thread thread;
    std::mutex mutex;
    bool is_closed = false;
    std::condition_variable condition_variable;
    std::vector<FramebufferInfo> frame_list;
    std::deque<std::size_t> writable_frame_queue;
    std::deque<std::size_t> readable_frame_queue;
};

class St2110TxAudioSession : public std::enable_shared_from_this<St2110TxAudioSession> {
public:
    struct FramebufferInfo {
        void *data = nullptr;
        std::optional<struct st30_tx_frame_meta> meta;
    };

    void init(const seeder::config::ST2110OutputConfig &in_output_config) {
        auto &output_config = in_output_config;
        auto main_ctx = weak_main_ctx.lock();

        memset(&ops, 0, sizeof(ops));
        short_name.append("ta_").append(output_config.audio.ip);
        ops.name = short_name.c_str();
        ops.priv = this;
        ops.num_port = 1;
        strncpy(ops.port[MTL_PORT_P], main_ctx->get_port(output_config.device_id).c_str(), MTL_PORT_MAX_LEN);
        inet_pton(AF_INET, output_config.audio.ip.c_str(), ops.dip_addr[MTL_PORT_P]);
        ops.udp_port[MTL_PORT_P] = output_config.video.port;
        if (output_config.audio.redundant.has_value() && output_config.audio.redundant->enable) {
            ops.num_port = 2;
            strncpy(ops.port[MTL_PORT_R], main_ctx->get_port(output_config.audio.redundant->device_id).c_str(), MTL_PORT_MAX_LEN);
            inet_pton(AF_INET, output_config.audio.redundant->ip.c_str(), ops.dip_addr[MTL_PORT_R]);
            ops.udp_port[MTL_PORT_R] = output_config.audio.redundant->port;
        }
        ops.type = ST30_TYPE_FRAME_LEVEL;
        ops.fmt = parse_value_from_desc_name_or_throw(
            st30_fmt_desc_list, 
            output_config.audio.audio_format, 
            "audio_format");
        ops.payload_type = output_config.audio.payload_type;
        ops.channel = parse_value_from_desc_name_or_throw(
            audio_channel_desc_list,
            output_config.audio.audio_channel,
            "audio_channel"
        );
        ops.sampling = parse_value_from_desc_name_or_throw(
            st30_sampling_desc_list,
            output_config.audio.audio_sampling,
            "audio_sampling"
        );
        ops.ptime = parse_value_from_desc_name_or_throw(
            st30_ptime_desc_list,
            parse_audio_ptime(output_config.audio),
            "audio_ptime"
        );
        // ops.sample_size = st30_get_sample_size(ops.fmt);
        // ops.sample_num = st30_get_sample_num(ops.ptime, ops.sampling);
        ops.framebuff_size = calc_st30_frame_size(ops.fmt, ops.ptime, ops.sampling, ops.channel);
        ops.framebuff_cnt = framebuff_cnt;

        ops.get_next_frame = get_next_frame_static;
        ops.notify_frame_done = frame_done_static;

        frame_bytes_size = ops.framebuff_size;

        frame_list.resize(framebuff_cnt);
    }

    void start() {
        logger->info("St2110TxAudioSession {} start", short_name);
        auto main_ctx = weak_main_ctx.lock();

        handle = st30_tx_create(main_ctx->handle, &ops);
        if (!handle) {
            throw std::runtime_error("st30_tx_create");
        }
        for (std::size_t i = 0; i < frame_list.size(); ++i) {
            auto &frame_info = frame_list[i];
            frame_info.data = st30_tx_get_framebuffer(handle, i);
            writable_frame_queue.push_back(i);
        }
        auto shared_this = shared_from_this();
        is_started = true;
        thread = std::thread([shared_this] {
            shared_this->run();
        });
    }

    void run() {
        seeder::core::util::set_thread_name(short_name);

        {
            std::unique_lock<std::mutex> lock(mutex);
            while (!is_closed) {
                std::size_t writable_frame_index;
                if (writable_frame_queue.empty()) {
                    condition_variable.wait(lock);
                    continue;
                }
                writable_frame_index = writable_frame_queue.front();
                writable_frame_queue.pop_front();
                lock.unlock();

                auto &frame = frame_list[writable_frame_index];
                if (!output->wait_get_audio_frame(frame.data)) {
                    break;
                }

                lock.lock();
                readable_frame_queue.push_back(writable_frame_index);
            }
        }
        logger->info("St2110TxAudioSession {} run finish", short_name);
    }

    void close() {
        if (!is_started) {
            return;
        }
        logger->info("St2110TxAudioSession {} close", short_name);

        is_started = false;
        {
            std::unique_lock<std::mutex> lock(mutex);
            is_closed = true;
            condition_variable.notify_all();
        }
        thread.join();

        int ret = st30_tx_free(handle);
        if (ret < 0) {
            logger->error("st30_tx_free failed: {}", ret);
        }
        handle = 0;
    }

    static int get_next_frame_static(
        void* priv, 
        uint16_t* next_frame_idx, 
        struct st30_tx_frame_meta* meta) {
        St2110TxAudioSession *s = (St2110TxAudioSession *) priv;
        if (s->require_next_frame(next_frame_idx)) {
            return 0;
        } else {
            return -EIO;
        }
    }

    bool require_next_frame(uint16_t *next_frame_idx) {
        std::unique_lock<std::mutex> lock(mutex);
        if (readable_frame_queue.empty()) {
            return false;
        }
        std::size_t frame_index = readable_frame_queue.front();
        readable_frame_queue.pop_front();
        *next_frame_idx = frame_index;
        return true;
    }

    static int frame_done_static(
        void* priv, 
        uint16_t frame_idx,
        struct st30_tx_frame_meta* meta) {
        St2110TxAudioSession *s = (St2110TxAudioSession *) priv;
        s->frame_done(frame_idx);
        return 0;
    }

    void frame_done(uint16_t frame_idx) {
        std::unique_lock<std::mutex> lock(mutex);
        writable_frame_queue.push_back(frame_idx);
        condition_variable.notify_one();
    }

    bool is_started = false;
    std::size_t framebuff_cnt = 20;
    std::size_t frame_bytes_size = 0;
    std::weak_ptr<StMainContext> weak_main_ctx;
    st30_tx_handle handle;
    struct st30_tx_ops ops;
    std::string short_name;
    std::shared_ptr<St2110BaseOutput> output;
    std::thread thread;
    std::mutex mutex;
    bool is_closed = false;
    std::condition_variable condition_variable;
    std::vector<FramebufferInfo> frame_list;
    std::deque<std::size_t> writable_frame_queue;
    std::deque<std::size_t> readable_frame_queue;
};

class St2110TxSession::Impl {
public:
    ~Impl() {
        close();
    }

    void start() {
        auto lock = acquire_lock();
        if (video_session) {
            video_session->start();
        }
        if (audio_session) {
            audio_session->start();
        }
    }

    void close() {
        auto lock = acquire_lock();
        if (video_session) {
            video_session->close();
            video_session = nullptr;
        }
        if (audio_session) {
            audio_session->close();
            audio_session = nullptr;
        }
    }

    std::unique_lock<std::mutex> acquire_lock() {
        return std::unique_lock<std::mutex>(mutex);
    }

    std::mutex mutex;
    std::string id;
    std::shared_ptr<St2110BaseOutput> output;
    std::shared_ptr<St2110TxVideoSession> video_session;
    std::shared_ptr<St2110TxAudioSession> audio_session;
};

St2110TxSession::St2110TxSession(): p_impl(new Impl) {}

St2110TxSession::~St2110TxSession() {}

void St2110TxSession::start() {
    p_impl->start();
}

void St2110TxSession::stop() {
    p_impl->close();
}

std::shared_ptr<St2110BaseOutput> St2110TxSession::get_output() {
    return p_impl->output;
}


class St2110RxSession::Impl {
public:
    ~Impl() {
        close();
    }

    void start() {
        auto lock = acquire_lock();
        if (video_session) {
            video_session->start();
        }
        if (audio_session) {
            audio_session->start();
        }
    }

    void close() {
        auto lock = acquire_lock();
        if (video_session) {
            video_session->close();
        }
        if (audio_session) {
            audio_session->close();
        }
    }

    std::unique_lock<std::mutex> acquire_lock() {
        return std::unique_lock<std::mutex>(mutex);
    }

    std::mutex mutex;
    std::string id;
    std::shared_ptr<St2110BaseSource> source;
    std::shared_ptr<St2110RxVideoSession> video_session;
    std::shared_ptr<St2110RxAudioSession> audio_session;
};

St2110RxSession::St2110RxSession(): p_impl(new Impl) {}

St2110RxSession::~St2110RxSession() {}

void St2110RxSession::start() {
    p_impl->start();
}

void St2110RxSession::stop() {
    p_impl->close();
}

std::shared_ptr<St2110BaseSource> St2110RxSession::get_source() {
    return p_impl->source;
}

void St2110RxSession::set_video_buffer_allocator(VideoBufferAllocator in_allocator) {
    if (p_impl->video_session) {
        p_impl->video_session->rx_frame_allocator = in_allocator;
    }
}

class St2110SessionManager::Impl {
public:
    Impl() {
        main_ctx.reset(new StMainContext);
    }

    void init(const seeder::config::ST2110Config &config) {
        logger = seeder::core::get_logger("st2110");
        if (!config.enable) {
            logger->info("st2110 disabled");
            return;
        }
        main_ctx->init(config);
        main_ctx->start();
    }

    void close() {
        for (auto &tx_session : tx_sessions) {
            tx_session->p_impl->close();
        }
        tx_sessions.clear();
        for (auto &rx_session : rx_sessions) {
            rx_session->p_impl->close();
        }
        rx_sessions.clear();

        main_ctx->close();
    }

    std::shared_ptr<St2110TxSession> create_tx_session(
        const seeder::config::ST2110OutputConfig &output_config) {
            
        if (!main_ctx->is_started) {
            logger->warn("create_tx_session main_ctx is not started");
            return nullptr;
        }
        auto format_desc = seeder::core::video_format_desc::get(output_config.video.video_format);
        if (format_desc.is_invalid()) {
            return nullptr;
        }
        setup_st2110_audio_format(format_desc, output_config.audio);

        std::shared_ptr<St2110TxSession> tx_session(new St2110TxSession);
        tx_session->p_impl->id = output_config.id;
        tx_session->p_impl->output = output_factory(output_config);

        if (output_config.video.enable) {
            std::shared_ptr<St2110TxVideoSession> tx_video_session(new St2110TxVideoSession);
            tx_video_session->weak_main_ctx = main_ctx;
            tx_video_session->output = tx_session->p_impl->output;
            tx_video_session->init(output_config);
            tx_session->p_impl->video_session = tx_video_session;
        }
        if (output_config.audio.enable) {
            std::shared_ptr<St2110TxAudioSession> tx_audio_session(new St2110TxAudioSession);
            tx_audio_session->weak_main_ctx = main_ctx;
            tx_audio_session->output = tx_session->p_impl->output;
            tx_audio_session->init(output_config);
            tx_session->p_impl->audio_session = tx_audio_session;
        }

        tx_sessions.push_back(tx_session);

        return tx_session;
    }

    std::shared_ptr<St2110RxSession> create_rx_session(
        const seeder::config::ST2110InputConfig &input_config) {
        if (!main_ctx->is_started) {
            logger->warn("create_rx_session main_ctx is not started");
            return nullptr;
        }
        auto format_desc = seeder::core::video_format_desc::get(input_config.video.video_format);
        if (format_desc.is_invalid()) {
            return nullptr;
        }
        setup_st2110_audio_format(format_desc, input_config.audio);

        std::shared_ptr<St2110RxSession> rx_session(new St2110RxSession);
        rx_session->p_impl->id = input_config.id;
        rx_session->p_impl->source = source_factory(input_config);
        rx_session->set_video_buffer_allocator(rx_video_buffer_allocator);
        if (input_config.video.enable) {
            std::shared_ptr<St2110RxVideoSession> rx_video_session(new St2110RxVideoSession);
            rx_video_session->weak_main_ctx = main_ctx;
            rx_video_session->source = rx_session->p_impl->source;
            rx_video_session->init(input_config);

            rx_session->p_impl->video_session = rx_video_session;
        }
        if (input_config.audio.enable) {
            std::shared_ptr<St2110RxAudioSession> rx_audio_session(new St2110RxAudioSession);
            rx_audio_session->weak_main_ctx = main_ctx;
            rx_audio_session->source = rx_session->p_impl->source;
            rx_audio_session->init(input_config);

            rx_session->p_impl->audio_session = rx_audio_session;
        }

        rx_sessions.push_back(rx_session);

        return rx_session;
    }

    void remove_tx_session(const std::string &id) {
        for (auto iter = tx_sessions.begin(); iter != tx_sessions.end(); ) {
            std::shared_ptr<St2110TxSession> tx_session = *iter;
            if (tx_session->p_impl->id == id) {
                tx_session->stop();

                iter = tx_sessions.erase(iter);
            } else {
                ++iter;
            }
        }
    }
    
    void remove_rx_session(const std::string &id) {
        for (auto iter = rx_sessions.begin(); iter != rx_sessions.end(); ) {
            std::shared_ptr<St2110RxSession> rx_session = *iter;
            if (rx_session->p_impl->id == id) {
                rx_session->stop();

                iter = rx_sessions.erase(iter);
            } else {
                ++iter;
            }
        }
    }

    void rx_video_session_change_ip(const seeder::config::ST2110InputConfig &input_config) {
        for (auto &rx_session : rx_sessions) {
            if (rx_session->p_impl->id == input_config.id && 
                rx_session->p_impl->video_session) {
                auto rx_lock = rx_session->p_impl->acquire_lock();
                if (rx_session->p_impl->video_session->is_started) {
                    st_rx_source_info rx_source_info {};
                    inet_pton(AF_INET, input_config.video.ip.data(), rx_source_info.sip_addr[MTL_PORT_P]);
                    rx_source_info.udp_port[MTL_PORT_P] = input_config.video.port;
                    if (input_config.video.redundant.has_value() && input_config.video.redundant->enable) {
                        inet_pton(AF_INET, input_config.video.redundant->ip.data(), rx_source_info.sip_addr[MTL_PORT_R]);
                        rx_source_info.udp_port[MTL_PORT_R] = input_config.video.redundant->port;
                    }
                    int ret = st20_rx_update_source(
                        rx_session->p_impl->video_session->handle,
                        &rx_source_info);
                    if (ret) {
                        logger->error("st20_rx_update_source ret={}", ret);
                    }
                } else {
                    rx_session->p_impl->video_session->assign_ip(input_config);
                }
            }
        }
    }

    std::shared_ptr<StMainContext> main_ctx;
    std::list<std::shared_ptr<St2110TxSession>> tx_sessions;
    std::list<std::shared_ptr<St2110RxSession>> rx_sessions;
    std::function<std::shared_ptr<St2110BaseSource>(const seeder::config::ST2110InputConfig &)> source_factory;
    std::function<std::shared_ptr<St2110BaseOutput>(const seeder::config::ST2110OutputConfig &)> output_factory;
    St2110RxSession::VideoBufferAllocator rx_video_buffer_allocator;
};

std::shared_ptr<St2110RxSession> St2110SessionManager::get_rx_session(const std::string &id) const {
    for (auto &rx_session : p_impl->rx_sessions) {
        if (rx_session->p_impl->id == id) {
            return rx_session;
        }
    }
    return nullptr;
}

St2110SessionManager::St2110SessionManager(): p_impl(new Impl) {}

St2110SessionManager::~St2110SessionManager() {}

void St2110SessionManager::init(const seeder::config::ST2110Config &config) {
    p_impl->init(config);
}

void St2110SessionManager::set_output_factory(std::function<std::shared_ptr<St2110BaseOutput>(const seeder::config::ST2110OutputConfig &)> in_factory) {
    p_impl->output_factory = in_factory;
}

std::shared_ptr<St2110TxSession> St2110SessionManager::create_tx_session(
    const seeder::config::ST2110OutputConfig &output_config) {
    return p_impl->create_tx_session(output_config);
}

void St2110SessionManager::remove_tx_session(const std::string &id) {
    p_impl->remove_tx_session(id);
}

void St2110SessionManager::set_source_factory(std::function<std::shared_ptr<St2110BaseSource>(const seeder::config::ST2110InputConfig &)> in_factory) {
    p_impl->source_factory = in_factory;
}

std::shared_ptr<St2110RxSession> St2110SessionManager::create_rx_session(
    const seeder::config::ST2110InputConfig &input_config) {
    return p_impl->create_rx_session(input_config);
}

void St2110SessionManager::close() {
    p_impl->close();
}

void St2110SessionManager::remove_rx_session(const std::string &id) {
    p_impl->remove_rx_session(id);
}

void St2110SessionManager::set_rx_video_buffer_allocator(St2110RxSession::VideoBufferAllocator in_allocator) {
    p_impl->rx_video_buffer_allocator = in_allocator;
}


void St2110SessionManager::rx_video_session_change_ip(const seeder::config::ST2110InputConfig &input_config) {
    p_impl->rx_video_session_change_ip(input_config);
}

void setup_st2110_audio_format(
    seeder::core::video_format_desc &format_desc,
    const seeder::config::ST2110AudioConfig &audio_config
) {
    
    enum st30_fmt audio_format = parse_value_from_desc_name_or_throw(
        st30_fmt_desc_list, 
        audio_config.audio_format.empty() ? "PCM24" : audio_config.audio_format, 
        "audio_format");
    int audio_channel = parse_value_from_desc_name_or_throw(
        audio_channel_desc_list,
        audio_config.audio_channel.empty() ? "ST": audio_config.audio_channel,
        "audio_channel"
    );
    enum st30_sampling audio_sampling = parse_value_from_desc_name_or_throw(
        st30_sampling_desc_list,
        audio_config.audio_sampling.empty() ? "48kHz" : audio_config.audio_sampling,
        "audio_sampling"
    );
    std::string ptime_str = parse_audio_ptime(audio_config);
    enum st30_ptime audio_ptime = parse_value_from_desc_name_or_throw(
        st30_ptime_desc_list,
        ptime_str,
        "audio_ptime"
    );
    format_desc.st30_ptime = boost::lexical_cast<double>(ptime_str);
    int sample_size = st30_get_sample_size(audio_format);
    format_desc.audio_channels = audio_channel;
    format_desc.sample_num = st30_get_sample_num(audio_ptime, audio_sampling);
    if(audio_ptime == ST30_PTIME_4MS) {
        format_desc.st30_frame_size = sample_size * st30_get_sample_num(ST30_PTIME_4MS, audio_sampling) * audio_channel;
        format_desc.st30_fps = 250;
    } else {
        format_desc.st30_frame_size = sample_size * st30_get_sample_num(ST30_PTIME_1MS, audio_sampling) * audio_channel;
        format_desc.st30_fps = 1000;
    }
    if (audio_sampling == ST30_SAMPLING_96K) {
        format_desc.audio_sample_rate = 96000;
    } else {
        format_desc.audio_sample_rate = 48000;
    }

    if (audio_format == ST30_FMT_PCM8)
        format_desc.audio_samples = 8;
    else if (audio_format == ST30_FMT_PCM24)
        format_desc.audio_samples = 24;
    else if (audio_format == ST31_FMT_AM824)
        format_desc.audio_samples = 32;
    else {
        format_desc.audio_samples = 16;
    }
}

std::size_t get_st2110_20_frame_size(const seeder::core::video_format_desc &format_desc) {
    return st20_frame_size(ST20_FMT_YUV_422_10BIT, format_desc.width, format_desc.height);
}

} // namespace seeder