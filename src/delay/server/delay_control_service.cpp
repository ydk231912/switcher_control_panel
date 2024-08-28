#include "delay_control_service.h"

#include <condition_variable>
#include <typeindex>
#include <unordered_map>
#include <deque>

#include <boost/asio.hpp>

#include "core/util/logger.h"
#include "core/video_format.h"
#include "server/http_service.h"
#include "util/io.h"
#include "util/json_util.h"
#include "util/stat_recorder.h"
#include "server/config_service.h"
#include "st2110/st2110_service.h"
#include "st2110/st2110_config.h"
#include "st2110/st2110_session.h"
#include "st2110/st2110_dummy.h"
#include "st2110/st2110_source.h"
#include "st2110/st2110_output.h"
#include "slice_service.h"
#include "slice_manager.h"


using seeder::core::logger;

namespace asio = boost::asio;

namespace seeder {

namespace {

struct BaseDeviceConfig {
    std::string id;
    std::string type;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        BaseDeviceConfig,
        id,
        type
    )
};

class AbstractDeviceContext {
public:
    virtual ~AbstractDeviceContext() {}

    virtual void start() noexcept {}

    virtual void stop() noexcept {}

    virtual const std::string & get_id() const noexcept = 0;

    // 仅初始化时调用一次
    virtual void set_device_config_any(const std::any &a_device_config) noexcept = 0;

    // 运行时调用，更新配置，并重启对应的设备
    virtual void update_device_config_any(const std::any &a_device_config) = 0;

    virtual std::any get_device_config_any() const noexcept = 0;

    virtual const BaseDeviceConfig & get_base_config() const noexcept = 0;
};

template<class D>
class BaseDeviceContext : public AbstractDeviceContext {
public:
    using DeviceConfig = D;

    const std::string & get_id() const noexcept override {
        return base_config.id;
    }

    void set_device_config_any(const std::any &a_device_config) noexcept override {
        const DeviceConfig *p_device_config = std::any_cast<DeviceConfig>(&a_device_config);
        if (p_device_config) {
            device_config = *p_device_config;
        } else {
            logger->warn("DeviceConfigFactory set_device_config_any {} any_cast failed: DeviceConfig={} any={}", base_config.type, typeid(DeviceConfig).name(), a_device_config.type().name());
        }
    }

    void update_device_config_any(const std::any &a_device_config) override {
        const DeviceConfig *p_device_config = std::any_cast<DeviceConfig>(&a_device_config);
        if (p_device_config) {
            update_device_config(*p_device_config);
        } else {
            logger->warn("DeviceConfigFactory update_device_config_any {} any_cast failed: DeviceConfig={} any={}", base_config.type, typeid(DeviceConfig).name(), a_device_config.type().name());
        }
    }

    virtual void update_device_config(const D &new_device_config) {
        device_config = new_device_config;
    }

    std::any get_device_config_any() const noexcept override {
        return device_config;
    }

    void set_base_config(const BaseDeviceConfig &in_base_config) {
        base_config = in_base_config;
    }

    const BaseDeviceConfig & get_base_config() const noexcept override {
        return base_config;
    }

protected:
    BaseDeviceConfig base_config;
    D device_config;
};

class DeviceConfigFactory {
public:
    struct Entry {
        std::string type_name;

        virtual ~Entry() {}

        virtual void to_json(nlohmann::json &j, const std::any &a) = 0;

        virtual void from_json(const nlohmann::json &j, std::any &a) = 0;

        virtual std::shared_ptr<AbstractDeviceContext> create_context(const BaseDeviceConfig &in_base_config) = 0;

        virtual void post_create_context(AbstractDeviceContext &in_device_ctx) = 0;

        void to_json_full(nlohmann::json &j, const std::any &a_device_config, const BaseDeviceConfig &base_config) {
            to_json(j, a_device_config);
            nlohmann::json base_j = base_config;
            j.update(base_j);
        }
    };

    template<class T>
    struct EntryImpl : public Entry {
        using DeviceConfig = typename T::DeviceConfig;

        void to_json(nlohmann::json &j, const std::any &a) override {
            const DeviceConfig *p = std::any_cast<DeviceConfig>(&a);
            if (p) {
                j = *p;
            } else {
                logger->warn("DeviceConfigFactory to_json {} any_cast failed: DeviceConfig={} any={}", type_name, typeid(DeviceConfig).name(), a.type().name());
            }
        }

        void from_json(const nlohmann::json &j, std::any &a) override {
            DeviceConfig value = j;
            a = value;
        }

        std::shared_ptr<AbstractDeviceContext> create_context(const BaseDeviceConfig &in_base_config) override {
            std::shared_ptr<T> p(new T);
            p->set_base_config(in_base_config);
            return p;
        }

        void post_create_context(AbstractDeviceContext &in_device_ctx) override {
            if (on_post_create_context) {
                auto device_ctx = dynamic_cast<T *>(&in_device_ctx);
                if (device_ctx) {
                    on_post_create_context(*device_ctx);
                }
            }
        }

        std::function<void(T &)> on_post_create_context;
    };

    template<class T>
    std::shared_ptr<EntryImpl<T>> add(const std::string &type_name) {
        using DeviceConfig = typename T::DeviceConfig;

        std::shared_ptr<EntryImpl<T>> entry(new EntryImpl<T>());
        entry->type_name = type_name;
        type_name_map[type_name] = entry;
        type_index_map[std::type_index(typeid(DeviceConfig))] = entry;

        return entry;
    }

    Entry * find(const std::string &name) {
        auto iter = type_name_map.find(name);
        if (iter == type_name_map.end()) {
            return nullptr;
        }
        return iter->second.get();
    }

    Entry * find(const std::any &a_device_config) {
        if (!a_device_config.has_value()) {
            return nullptr;
        }
        auto iter = type_index_map.find(std::type_index(a_device_config.type()));
        if (iter == type_index_map.end()) {
            return nullptr;
        }
        return iter->second.get();
    }

    std::string parse_any_id(const std::any &a_device_config) {
        Entry *e = find(a_device_config);
        std::string id;
        if (e) {
            nlohmann::json j;
            e->to_json(j, a_device_config);
            seeder::json::object_safe_get_to(j, "id", id);
        }
        return id;
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<Entry>> type_index_map;
    std::unordered_map<std::string, std::shared_ptr<Entry>> type_name_map;
};

class St2110InputDeviceContext : public BaseDeviceContext<seeder::config::ST2110InputConfig> {
public:

    void start() noexcept override {
        try {
            session = st2110_service->get_session_manager()->create_rx_session(device_config);
            if (session) {
                session->start();
            }
        } catch (std::exception &e) {
            logger->error("St2110InputDeviceContext::start error: {}", e.what());
            session = nullptr;
        }
    }

    void stop() noexcept override {
        if (session) {
            session->stop();
            session = nullptr;
        }
    }

    const seeder::config::ST2110InputConfig & get_device_config() const {
        return device_config;
    }

    void update_device_config(const seeder::config::ST2110InputConfig &new_config) override {
        this->stop();
        st2110_service->get_session_manager()->remove_rx_session(new_config.id);
        device_config = new_config;
        this->start();
    }

    St2110Service *st2110_service = nullptr;
    std::shared_ptr<St2110RxSession> session;
};

class St2110DummyFileInputContext : public BaseDeviceContext<seeder::config::St2110DummyFileInputConfig> {
public:
    void start() noexcept override {
        try {
            input.reset(new St2110DummyFileInput(device_config));
        } catch (std::exception &e) {
            logger->error("St2110DummyFileInputContext start error: {}", e.what());
        }
    }

    void stop() noexcept override {
        input = nullptr;
    }

    const seeder::config::St2110DummyFileInputConfig & get_device_config() const {
        return device_config;
    }

    std::shared_ptr<St2110DummyFileInput> input;
};

class St2110OutputDeviceContext : public BaseDeviceContext<seeder::config::ST2110OutputConfig> {
public:

    void start() noexcept override {
        try {
            session = st2110_service->get_session_manager()->create_tx_session(device_config);
            if (session) {
                session->start();
            }
        } catch (std::exception &e) {
            logger->error("St2110OutputDeviceContext::start error: {}", e.what());
            session = nullptr;
        }
    }

    void stop() noexcept override {
        if (session) {
            session->stop();
            session = nullptr;
        }
    }

    const seeder::config::ST2110OutputConfig & get_device_config() const {
        return device_config;
    }

    void update_device_config(const seeder::config::ST2110OutputConfig &new_config) override {
        this->stop();
        st2110_service->get_session_manager()->remove_tx_session(new_config.id);
        device_config = new_config;
        this->start();
    }

    St2110Service *st2110_service = nullptr;
    std::shared_ptr<St2110TxSession> session;
};

class ErrorCodeCollector : public seeder::AbstractStatRecorder {
public:
    explicit ErrorCodeCollector(const std::string &in_name): name(in_name) {}

    void dump() override {
        std::unique_lock<std::mutex> lock(mutex);
        if (error_map.empty()) {
            return;
        }
        logger->warn("{}: {}", name, dump_slice_errors(error_map));
    }

    void reset() override {
        std::unique_lock<std::mutex> lock(mutex);
        error_map.clear();
    }

    void record(const std::error_code &ec) {
        std::unique_lock<std::mutex> lock(mutex);
        error_map[ec]++;
    }

    static std::string dump_slice_errors(std::unordered_map<std::error_code, uint32_t> &error_map) {
        std::string s;
        bool is_first = true;
        for (auto &p : error_map) {
            if (!is_first) {
                s.append(",");
            }
            s.append(p.first.message()).append("=").append(std::to_string(p.second));
            is_first = false;
        }
        error_map.clear();
        return s;
    }

    static std::shared_ptr<ErrorCodeCollector> create(const std::string &name) {
        std::shared_ptr<ErrorCodeCollector> r(new ErrorCodeCollector(name));
        StatRecorderRegistry::get_instance().add_recorder(r);
        return r;
    }

    std::string name;
    std::mutex mutex;
    std::unordered_map<std::error_code, uint32_t> error_map;
};

class St2110SourceImpl : public St2110BaseSource {
public:
    bool push_video_frame(seeder::core::AbstractBuffer &in_buffer) override {
        std::unique_lock<std::mutex> video_lock(video_mutex);
        if (slice_manager) {
            std::error_code ec;
            if (audio_frame_size) {
                muxer_audio_buffer.resize(audio_frame_size * audio_frame_count);
                {
                    std::unique_lock<std::mutex> audio_lock(audio_mutex);
                    size_t len = std::min(audio_frame_size * audio_frame_count, audio_data.size());
                    if (len) {
                        memcpy(muxer_audio_buffer.data(), audio_data.data(), len);
                        audio_data.erase(audio_data.begin(), audio_data.begin() + len);
                    }
                }
                buffers = {
                    asio::buffer(in_buffer.get_data(), video_frame_size),
                    asio::buffer(muxer_audio_buffer)
                };
                ec = slice_manager->write_slice_block(buffers);
                muxer_audio_buffer.clear();
            } else {
                // video only
                ec = slice_manager->write_slice_block(asio::buffer(in_buffer.get_data(), video_frame_size));
            }
            if (ec) {
                slice_errors->record(ec);
            }
        }
        return true;
    }

    void push_audio_frame(seeder::core::AbstractBuffer &in_buffer) override {
        std::unique_lock<std::mutex> audio_lock(audio_mutex);
        if (audio_frame_size) {
            if (audio_data.size() < audio_frame_size * (audio_frame_count + extra_audio_frame_count)) {
                const uint8_t *src_data = (uint8_t *)in_buffer.get_data();
                size_t len = std::min(audio_frame_size * (audio_frame_count + extra_audio_frame_count) - audio_data.size(), in_buffer.get_size());
                audio_data.insert(audio_data.end(), src_data, src_data + len);
            } else {
                SEEDER_STATIC_STAT_RECORDER_SIMPLE_VALUE_NO_ZERO(int, g_stat_frame_drop, warn, "St2110SourceImpl.audio_frame_drop");
                ++g_stat_frame_drop->get_atomic();
            }
        }
    }

    void set_slice_manager(
        const std::shared_ptr<SliceManager> &in_slice_manager,
        size_t in_video_frame_size,
        size_t in_audio_frame_size,
        size_t in_audio_frame_count
    ) {
        std::unique_lock<std::mutex> video_lock(video_mutex);
        std::unique_lock<std::mutex> audio_lock(audio_mutex);
        slice_manager = in_slice_manager;
        video_frame_size = in_video_frame_size;
        audio_frame_size = in_audio_frame_size;
        audio_frame_count = in_audio_frame_count;
        audio_data.clear();
        audio_data.reserve(audio_frame_size * (audio_frame_count + extra_audio_frame_count));
        muxer_audio_buffer.clear();
        muxer_audio_buffer.reserve(audio_frame_size * audio_frame_count);
        slice_errors = ErrorCodeCollector::create("St2110SourceImpl.write_slice_block");
    }

    void close() {
        std::unique_lock<std::mutex> video_lock(video_mutex);
        std::unique_lock<std::mutex> audio_lock(audio_mutex);
        slice_manager = nullptr;
    }

private:
    size_t video_frame_size = 0;
    size_t audio_frame_size = 0;
    size_t audio_frame_count = 0;
    std::mutex video_mutex;
    std::mutex audio_mutex;
    std::shared_ptr<SliceManager> slice_manager;
    std::vector<uint8_t> audio_data;
    std::vector<uint8_t> muxer_audio_buffer;
    std::vector<asio::const_buffer> buffers;
    std::shared_ptr<ErrorCodeCollector> slice_errors;

    static constexpr size_t extra_audio_frame_count = 3;
};

class SliceReaderHolder {
public:
    explicit SliceReaderHolder() {}

    explicit SliceReaderHolder(
        SliceManager *in_slice_manager,
        SliceReader *in_reader
    ): reader(in_reader)
    {}

    SliceReaderHolder(const SliceReaderHolder &) = delete;
    SliceReaderHolder & operator=(const SliceReaderHolder &) = delete;

    SliceReaderHolder(SliceReaderHolder &&other) {
        release();
        slice_manager = other.slice_manager;
        reader = other.reader;
    }

    SliceReaderHolder & operator=(SliceReaderHolder &&other) {
        release();
        slice_manager = other.slice_manager;
        reader = other.reader;
        return *this;
    }

    ~SliceReaderHolder() {
        release();
    }

    SliceReader * get() {
        return reader;
    }

private:
    void release() {
        if (slice_manager && reader) {
            slice_manager->remove_slice_cursor(reader);
            reader = nullptr;
        }
    }

private:
    SliceManager *slice_manager = nullptr;
    SliceReader *reader = nullptr;
};

class St2110OutputImpl : public St2110BaseOutput {
public:
    bool wait_get_video_frame(void *dst) override {
        std::unique_lock<std::mutex> video_lock(video_mutex);
        cv.wait(video_lock, [this] {
            return is_closed || slice_manager;
        });
        if (is_closed) {
            return false;
        }
        std::error_code ec;
        SliceReader *reader = is_bypass_enable ? bypass_slice.get() : delay_slice.get();
        if (!reader) {
            return true;
        }
        if (audio_frame_size) {
            demuxer_audio_buffer.resize(audio_frame_size * audio_frame_count, 0);
            buffers = {
                asio::buffer(dst, video_frame_size),
                asio::buffer(demuxer_audio_buffer)
            };
            ec = slice_manager->read_slice_block(reader, buffers);
            {
                std::unique_lock<std::mutex> audio_lock(audio_mutex);
                std::swap(demuxer_audio_buffer, audio_data);
                audio_packets.clear();
                for (size_t i = 0; i < audio_frame_count; ++i) {
                    audio_packets.push_back(audio_data.data() + audio_frame_size * i);
                }
            }
        } else {
            // video only
            ec = slice_manager->read_slice_block(reader, asio::buffer(dst, video_frame_size));
        }
        if (ec) {
            slice_errors->record(ec);
        }

        return true;
    }

    bool wait_get_audio_frame(void *dst) override {
        std::unique_lock<std::mutex> audio_lock(audio_mutex);
        cv.wait(audio_lock, [this] {
            return is_closed || slice_manager;
        });
        if (is_closed) {
            return false;
        }
        if (!audio_packets.empty()) {
            memcpy(dst, audio_packets.front(), audio_frame_size);
            audio_packets.pop_front();
        }

        return true;
    }

    void set_slice_manager(
        const std::shared_ptr<SliceManager> &in_slice_manager,
        size_t in_video_frame_size,
        size_t in_audio_frame_size,
        size_t in_audio_frame_count
    ) {
        std::unique_lock<std::mutex> video_lock(video_mutex);
        std::unique_lock<std::mutex> audio_lock(audio_mutex);
        slice_manager = in_slice_manager;
        bypass_slice = SliceReaderHolder(slice_manager.get(), slice_manager->add_read_cursor(1));
        video_frame_size = in_video_frame_size;
        audio_frame_size = in_audio_frame_size;
        audio_frame_count = in_audio_frame_count;
        audio_data.clear();
        audio_data.reserve(audio_frame_size * audio_frame_count);
        audio_packets.clear();
        demuxer_audio_buffer.clear();
        demuxer_audio_buffer.reserve(audio_frame_size * audio_frame_count);
        slice_errors = ErrorCodeCollector::create("St2110SourceImpl.read_slice_block");
        cv.notify_all();
    }

    void close() {
        std::unique_lock<std::mutex> video_lock(video_mutex);
        std::unique_lock<std::mutex> audio_lock(audio_mutex);
        slice_manager = nullptr;
        is_closed = true;
        cv.notify_all();
    }

    void set_bypass(bool in_bypass_enable) {
        std::unique_lock<std::mutex> video_lock(video_mutex);
        std::unique_lock<std::mutex> audio_lock(audio_mutex);
        is_bypass_enable = in_bypass_enable;
    }

    void set_read_cursor(int read_cursor) {
        std::unique_lock<std::mutex> video_lock(video_mutex);
        std::unique_lock<std::mutex> audio_lock(audio_mutex);
        delay_slice = SliceReaderHolder(slice_manager.get(), slice_manager->add_read_cursor(read_cursor));
    }

private:
    size_t video_frame_size = 0;
    size_t audio_frame_size = 0;
    size_t audio_frame_count = 0;
    std::mutex video_mutex;
    std::mutex audio_mutex;
    std::condition_variable cv;
    bool is_closed = false;
    std::shared_ptr<SliceManager> slice_manager;
    SliceReaderHolder delay_slice;
    SliceReaderHolder bypass_slice;
    bool is_bypass_enable = false;
    std::vector<uint8_t> audio_data;
    std::deque<uint8_t *> audio_packets;
    std::vector<uint8_t> demuxer_audio_buffer;
    std::vector<asio::mutable_buffer> buffers;
    std::shared_ptr<ErrorCodeCollector> slice_errors;
};

struct DelayStreamConfig {
    std::string main_input_id;
    std::string main_output_id;
    // std::string preview_output_id;
    int delay_duration_sec = 0;
    int delay_duration_frames = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        DelayStreamConfig,
        main_input_id,
        main_output_id,
        delay_duration_sec,
        delay_duration_frames
    )
};

class DelayStream {
public:

    ~DelayStream() {
        if (st2110_source) {
            st2110_source->close();
        }
        if (st2110_output) {
            st2110_output->close();
        }
    }

    void set_bypass(bool in_bypass_enable) {
        is_bypass_enable = in_bypass_enable;
        if (st2110_output) {
            st2110_output->set_bypass(in_bypass_enable);
        }
    }

    DelayStreamConfig config;
    double fps = 0;
    bool is_bypass_enable = false;
    std::shared_ptr<SliceManager> slice_manager;
    std::shared_ptr<St2110SourceImpl> st2110_source;
    std::shared_ptr<St2110OutputImpl> st2110_output;
};

} // namespace seeder::{}

class DelayControlService::Impl {
public:
    void setup() {
        io_ctx.reset(new asio::io_context);
        setup_config();
        setup_http();
        setup_device_config_factory();
    }

    void setup_config() {
        inputs_config_proxy.init_add(*config_service, "inputs");
        outputs_config_proxy.init_add(*config_service, "outputs");
        delay_stream_config_proxy.init_add(*config_service, "delay_stream");
    }

    void setup_http();

    void setup_device_config_factory() {
        auto st2110_input = input_device_config_factory.add<St2110InputDeviceContext>("st2110");
        st2110_input->on_post_create_context = [this] (St2110InputDeviceContext &device_ctx) {
            device_ctx.st2110_service = st2110_service.get();
        };
        input_device_config_factory.add<St2110DummyFileInputContext>("st2110_dummy_file");
        auto st2110_output = output_device_config_factory.add<St2110OutputDeviceContext>("st2110");
        st2110_output->on_post_create_context = [this] (St2110OutputDeviceContext &device_ctx) {
            device_ctx.st2110_service = st2110_service.get();
        };
    }

    void startup() {
        startup_st2110();
        startup_inputs();
        startup_outputs();
        reset_delay_stream();
    }

    void startup_st2110() {
        auto session_manager = st2110_service->get_session_manager();
        session_manager->set_source_factory([] (const seeder::config::ST2110InputConfig &input_config) {
            std::shared_ptr<St2110BaseSource> result;
            result.reset(new St2110SourceImpl);
            return result;
        });
        session_manager->set_output_factory([] (const seeder::config::ST2110OutputConfig &output_config) {
            std::shared_ptr<St2110BaseOutput> result;
            result.reset(new St2110OutputImpl);
            return result;
        });
    }

    void startup_inputs() {
        std::vector<seeder::json::BasePropsWrapper<BaseDeviceConfig>> inputs;
        inputs_config_proxy.try_get_to(inputs);
        for (auto &input_config : inputs) {
            auto entry = input_device_config_factory.find(input_config.base.type);
            if (!entry) {
                logger->warn("DelayControlService::Impl::startup wrong type: {}", input_config.base.type);
                continue;
            }
            std::any a_device_config;
            entry->from_json(input_config.props, a_device_config);
            auto input = entry->create_context(input_config.base);
            if (input) {
                entry->post_create_context(*input);
                input->set_device_config_any(a_device_config);
                add_input(input);
            }
        }
        logger->debug("DelayControlService startup_inputs finish");
    }

    void startup_outputs() {
        std::vector<seeder::json::BasePropsWrapper<BaseDeviceConfig>> outputs;
        outputs_config_proxy.try_get_to(outputs);
        for (auto &output_config : outputs) {
            auto entry = output_device_config_factory.find(output_config.base.type);
            if (!entry) {
                logger->warn("DelayControlService::Impl::startup wrong type: {}", output_config.base.type);
                continue;
            }
            std::any a_device_config;
            entry->from_json(output_config.props, a_device_config);
            auto output = entry->create_context(output_config.base);
            if (output) {
                entry->post_create_context(*output);
                output->set_device_config_any(a_device_config);
                add_output(output);
            }
        }
        logger->debug("DelayControlService startup_outputs finish");
    }

    void reset_delay_stream() {
        DelayStreamConfig delay_stream_config;
        delay_stream_config_proxy.try_get_to(delay_stream_config);

        auto input_ctx = find_input(delay_stream_config.main_input_id);
        auto st2110_input_ctx = std::dynamic_pointer_cast<St2110InputDeviceContext>(input_ctx);
        if (!st2110_input_ctx) {
            return;
        }
        auto st2110_source = std::dynamic_pointer_cast<St2110SourceImpl>(st2110_input_ctx->session->get_source());
        auto output_ctx = find_output(delay_stream_config.main_output_id);
        auto st2110_output_ctx = std::dynamic_pointer_cast<St2110OutputDeviceContext>(output_ctx);
        if (!st2110_output_ctx) {
            return;
        }
        auto st2110_output = std::dynamic_pointer_cast<St2110OutputImpl>(st2110_output_ctx->session->get_output());
        if (st2110_input_ctx->get_device_config().video.video_format != st2110_output_ctx->get_device_config().video.video_format) {
            logger->error("st2110 input video format != output video format");
            return;
        }
        auto &input_audio_config = st2110_input_ctx->get_device_config().audio;
        auto &output_audio_config = st2110_output_ctx->get_device_config().audio;
        if (input_audio_config.audio_format != output_audio_config.audio_format) {
            logger->error("st2110 input audio format != output audio format");
            return;
        }
        auto format_desc = seeder::core::video_format_desc::get(st2110_input_ctx->get_device_config().video.video_format);
        seeder::setup_st2110_audio_format(format_desc, input_audio_config);
        size_t video_frame_size = seeder::get_st2110_20_frame_size(format_desc);
        size_t audio_frame_size = format_desc.st30_frame_size;
        size_t audio_frame_count = std::ceil(1000 / format_desc.fps / format_desc.st30_ptime);
        size_t slice_block_size = video_frame_size + audio_frame_size * audio_frame_count;
        size_t slice_block_count = format_desc.fps;

        try {
            delay_stream.reset(new DelayStream);
            delay_stream->config = delay_stream_config;
            delay_stream->fps = format_desc.fps;
            delay_stream->slice_manager = slice_service->create_slice_manager(slice_block_size, slice_block_count);
            delay_stream->st2110_output = st2110_output;
            delay_stream->st2110_source = st2110_source;
            st2110_output->set_slice_manager(delay_stream->slice_manager, video_frame_size, audio_frame_size, audio_frame_count);
            st2110_output->set_read_cursor(delay_stream_config.delay_duration_sec * format_desc.fps + delay_stream_config.delay_duration_frames);
            st2110_source->set_slice_manager(delay_stream->slice_manager, video_frame_size, audio_frame_size, audio_frame_count);
        } catch (std::exception &e) {
            logger->error("startup_delay_stream: {}", e.what());
            delay_stream = nullptr;
        }
    }

    void add_input(const std::shared_ptr<AbstractDeviceContext> &input) {
        if (add_to_device_map(input_device_map, input)) {
            input->start();
            on_device_event(DeviceEventType::Added, input->get_device_config_any());
        }
    }

    void update_input(const std::any &a_device_config) noexcept {
        auto e = input_device_config_factory.find(a_device_config);
        if (!e) {
            return;
        }
        try {
            std::string id = input_device_config_factory.parse_any_id(a_device_config);
            auto input = find_input(id);
            if (!input) {
                logger->debug("DelayControlService update_input input_id={} not found", id);
                return;
            }
            reset_input(input, a_device_config);
            
            on_device_event(DeviceEventType::Updated, a_device_config);
            
            auto inputs = inputs_config_proxy.get();
            for (auto &input : inputs) {
                if (input.base.id == id) {
                    e->to_json(input.props, a_device_config);
                }
            }
            inputs_config_proxy.update(inputs);
        } catch (std::exception &e) {
            logger->error("DelayControlService update_input error: {}", e.what());       
        }
    }

    void reset_input(std::shared_ptr<AbstractDeviceContext> &input, const std::any &a_device_config) {
        input->update_device_config_any(a_device_config);
    }

    void add_output(const std::shared_ptr<AbstractDeviceContext> &output) {
        if (add_to_device_map(output_device_map, output)) {
            output->start();
            on_device_event(DeviceEventType::Added, output->get_device_config_any());
        }
    }

    void update_output(const std::any &a_device_config) {
        auto e = output_device_config_factory.find(a_device_config);
        if (!e) {
            return;
        }
        try {
            std::string id = output_device_config_factory.parse_any_id(a_device_config);
            auto output = find_output(id);
            if (!output) {
                logger->debug("DelayControlService update_output output_id={} not found", id);
                return;
            }

            reset_output(output, a_device_config);

            on_device_event(DeviceEventType::Updated, a_device_config);
            
            auto outputs = outputs_config_proxy.get();
            for (auto &output : outputs) {
                if (output.base.id == id) {
                    e->to_json(output.props, a_device_config);
                }
            }
            outputs_config_proxy.update(outputs);
        } catch (std::exception &e) {
            logger->error("DelayControlService update_output error: {}", e.what());       
        }
    }

    void reset_output(std::shared_ptr<AbstractDeviceContext> &output, const std::any &a_device_config) {
        output->update_device_config_any(a_device_config);
    }

    std::shared_ptr<AbstractDeviceContext> find_input(const std::string &id) const {
        auto iter = input_device_map.find(id);
        if (iter != input_device_map.end()) {
            return iter->second;
        }
        return nullptr;
    }

    std::shared_ptr<AbstractDeviceContext> find_output(const std::string &id) const {
        auto iter = output_device_map.find(id);
        if (iter != output_device_map.end()) {
            return iter->second;
        }
        return nullptr;
    }

    template<class T>
    bool add_to_device_map(std::unordered_map<std::string, std::shared_ptr<T>> &map, const std::shared_ptr<T> &device_ctx) {
        const std::string &id = device_ctx->get_id();
        if (map.find(id) != map.end()) {
            logger->error("DelayControlService add_to_device_map duplicate id: {}", id);
            return false;
        }
        map[id] = device_ctx;
        return true;
    }

    nlohmann::json get_device_info() const {
        nlohmann::json result;
        if (delay_stream) {
            auto &j_stream = result["delay_stream"];
            auto input_ctx = find_input(delay_stream->config.main_input_id);
            auto st2110_input_ctx = std::dynamic_pointer_cast<St2110InputDeviceContext>(input_ctx);
            if (st2110_input_ctx) {
                auto &input_config = st2110_input_ctx->get_device_config();
                auto &j_input = j_stream["main_input"];
                j_input = input_config;
            }
            
            auto output_ctx = find_output(delay_stream->config.main_output_id);
            auto st2110_output_ctx = std::dynamic_pointer_cast<St2110OutputDeviceContext>(output_ctx);
            if (st2110_output_ctx) {
                auto &output_config = st2110_output_ctx->get_device_config();
                auto &j_output = j_stream["main_output"];
                j_output["device_id"] = output_config.device_id;
                j_output = output_config;
            }
        }

        return result;
    }

    nlohmann::json get_delay_stream_config() const {
        nlohmann::json result;

        if (delay_stream) {
            result["delay_duration"] = nlohmann::json({
                {"sec_num", delay_stream->config.delay_duration_sec},
                {"frame_num", delay_stream->config.delay_duration_frames}
            });
            result["max_delay_duration"] = nlohmann::json({
                {"sec_num", slice_service->get_max_read_slice_count()}
            });
        }

        return result;
    }

    void update_delay_device_config(nlohmann::json &param) {
        DelayStreamConfig delay_stream_config;
        delay_stream_config_proxy.try_get_to(delay_stream_config);

        auto input_ctx = find_input(delay_stream_config.main_input_id);
        auto st2110_input_ctx = std::dynamic_pointer_cast<St2110InputDeviceContext>(input_ctx);
        if (!st2110_input_ctx) {
            return;
        }
        auto st2110_source = std::dynamic_pointer_cast<St2110SourceImpl>(st2110_input_ctx->session->get_source());
        auto output_ctx = find_output(delay_stream_config.main_output_id);
        auto st2110_output_ctx = std::dynamic_pointer_cast<St2110OutputDeviceContext>(output_ctx);
        if (!st2110_output_ctx) {
            return;
        }

        auto &j_stream = param["delay_stream"];
        bool should_reset_stream = false;
        auto &j_input = j_stream["main_input"];
        if (j_input.is_object()) {
            seeder::config::ST2110InputConfig new_input_config = j_input;
            update_input(new_input_config);
            should_reset_stream = true;
        }
        auto &j_output = j_stream["main_output"];
        if (j_output.is_object()) {
            seeder::config::ST2110OutputConfig new_output_config = j_output;
            update_output(new_output_config);
            should_reset_stream = true;
        }

        if (should_reset_stream) {
            reset_delay_stream();
        }
    }

    void update_delay_config(nlohmann::json &param) {
        if (param.contains("delay_duration") && delay_stream) {
            int sec_num = 0;
            int frame_num = 0;
            seeder::json::object_safe_get_to(param, "sec_num", sec_num);
            seeder::json::object_safe_get_to(param, "frame_num", frame_num);
            int max_frame_num = slice_service->get_max_read_slice_count() * delay_stream->fps;
            int new_frame_num = sec_num * delay_stream->fps + frame_num;
            int old_frame_num = delay_stream->config.delay_duration_sec * delay_stream->fps + delay_stream->config.delay_duration_frames;
            if (new_frame_num > 0 && new_frame_num <= max_frame_num && new_frame_num != old_frame_num) {
                delay_stream->config.delay_duration_sec = sec_num;
                delay_stream->config.delay_duration_frames = frame_num;
                delay_stream_config_proxy.update(delay_stream->config);
                delay_stream->st2110_output->set_read_cursor(new_frame_num);
            }
        }
    }

    void start() {
        asio::post(*io_ctx, [this] {
            try {
                startup();
            } catch (std::exception &e) {
                logger->error("DelayControlService startup error: {}", e.what());
            }
        });
        join_handle = seeder::async_run_with_io_context(io_ctx);
    }

    std::shared_ptr<ConfigService> config_service;
    std::shared_ptr<HttpService> http_service;
    std::shared_ptr<St2110Service> st2110_service;
    std::shared_ptr<SliceService> slice_service;

    ConfigCategoryProxy<std::vector<seeder::json::BasePropsWrapper<BaseDeviceConfig>>> inputs_config_proxy;
    ConfigCategoryProxy<std::vector<seeder::json::BasePropsWrapper<BaseDeviceConfig>>> outputs_config_proxy;

    std::unordered_map<std::string, std::shared_ptr<AbstractDeviceContext>> input_device_map;
    std::unordered_map<std::string, std::shared_ptr<AbstractDeviceContext>> output_device_map;

    DeviceConfigFactory input_device_config_factory;
    DeviceConfigFactory output_device_config_factory;

    ConfigCategoryProxy<DelayStreamConfig> delay_stream_config_proxy;
    std::shared_ptr<DelayStream> delay_stream;

    boost::signals2::signal<void(DeviceEventType, const std::any &)> on_device_event;

    JoinHandle join_handle;
    std::shared_ptr<asio::io_context> io_ctx;
};

DelayControlService::DelayControlService(): p_impl(new Impl) {}

DelayControlService::~DelayControlService() {}

void DelayControlService::setup(ServiceSetupCtx &ctx) {
    p_impl->config_service = ctx.get_service<ConfigService>();
    p_impl->st2110_service = ctx.get_service<St2110Service>();
    p_impl->http_service = ctx.get_service<HttpService>();
    p_impl->slice_service = ctx.get_service<SliceService>();
    p_impl->setup();
}

void DelayControlService::start() {
    p_impl->start();
}

boost::signals2::signal<void(DeviceEventType, const std::any &)> & DelayControlService::get_device_event_signal() {
    return p_impl->on_device_event;
}

#define POST_PROMISE(type, body, value) \
    auto promise = std::make_shared<std::promise<type>>();\
    asio::post(*p_impl->io_ctx, [=] {\
        try {\
            body;\
            promise->set_value(value);\
        } catch (...) {\
            promise->set_exception(std::current_exception());\
        }\
    });\
    return promise->get_future();

std::future<nlohmann::json> DelayControlService::get_device_info_async() {
    POST_PROMISE(nlohmann::json, {}, p_impl->get_device_info());
}

std::future<void> DelayControlService::update_device_config_async(const nlohmann::json &param) {
    POST_PROMISE(void, {
        auto j = param;
        p_impl->update_delay_device_config(j);
    }, );
}


#define HTTP_ROUTE(method, path, handler) \
    http_service->add_route_with_ctx(HttpMethod::method, path, [this] (const HttpRequest &req, HttpResponse &resp) handler, io_ctx)

void DelayControlService::Impl::setup_http() {
    HTTP_ROUTE(Get, "/api/delay/delay_status", {
        resp.json_ok(this->get_delay_stream_config());
    });

    HTTP_ROUTE(Post, "/api/delay/update_delay_config", {
        auto j = req.parse_json_body();
        this->update_delay_config(j);
        resp.json_ok();
    });
}


} // namespace seeder