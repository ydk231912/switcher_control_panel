#include "stat_service.h"

#include <boost/asio.hpp>

#include <nlohmann/json.hpp>

#include "server/config_servce.h"
#include "server/http_service.h"
#include "app_build_config.h"
#include "util/io.h"
#include "util/stat_recorder.h"

namespace asio = boost::asio;

namespace seeder {

namespace {

struct StatRecorderConfig {
    bool enable = true;
    int dump_interval_sec = 10;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        StatRecorderConfig,
        enable,
        dump_interval_sec
    )
};

struct StatRecordDumper {
    explicit StatRecordDumper(asio::io_context &in_io_ctx, const StatRecorderConfig &in_config): 
        io_ctx(in_io_ctx), config(in_config)
    {}

    void start() {
        if (config.dump_interval_sec <= 0) {
            return;
        }
        timer.reset(new asio::steady_timer(io_ctx));
        async_wait();
    }

    void async_wait() {
        timer->expires_after(std::chrono::seconds(config.dump_interval_sec));
        timer->async_wait([this] (const boost::system::error_code &ec) {
            if (ec) {
                return;
            }
            do_dump();
            async_wait();
        });
    }

    void do_dump() {
        StatRecorderRegistry::get_instance().dump();
    }

    asio::io_context &io_ctx;
    StatRecorderConfig config;
    std::unique_ptr<asio::steady_timer> timer;
};

} // namespace seeder::{}

class StatService::Impl {
public:
    void setup() {
        http_service->add_route(HttpMethod::Get, "/api/health", [this] (const HttpRequest &req, HttpResponse &resp) {
            nlohmann::json j;
            if (is_started) {
                j["startup_time"] = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startup_time).count();
                j["rev"] = PROJECT_GIT_HASH;
            }
            resp.json_ok(j);
        });
    }

    void start() {
        startup_time = std::chrono::system_clock::now();
        is_started = true;
        join_handle = seeder::async_run_with_new_io_context();
        io_ctx = join_handle.get_io_ctx();
        StatRecorderConfig config;
        stat_config_proxy.try_get_to(config);
        if (config.enable) {
            dumper.reset(new StatRecordDumper(*io_ctx, config));
            dumper->start();
        }
    }

    std::shared_ptr<HttpService> http_service;
    bool is_started = false;
    std::chrono::system_clock::time_point startup_time;
    ConfigCategoryProxy<StatRecorderConfig> stat_config_proxy;
    std::unique_ptr<StatRecordDumper> dumper;
    AsioJoinHandle join_handle;
    std::shared_ptr<asio::io_context> io_ctx;
};

StatService::StatService(): p_impl(new Impl) {}

StatService::~StatService() {}

void StatService::setup(ServiceSetupCtx &ctx) {
    auto config_service = ctx.get_service<ConfigService>();
    p_impl->stat_config_proxy.init_add(*config_service, "stat");
    p_impl->http_service = ctx.get_service<HttpService>();
    p_impl->setup();
}

void StatService::start() {
    p_impl->start();
}


} // namespace seeder
