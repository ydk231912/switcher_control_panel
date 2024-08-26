#include "app_ctx.h"

#include <exception>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>

#include "app_build_config.h"
#include "core/util/logger.h"

#include "server/service_manager.h"
#include "server/command_line_service.h"
#include "server/config_service.h"
#include "server/http_service.h"
#include "server/stat_service.h"
#include "st2110/st2110_service.h"
#include "server/delay_control_service.h"

namespace asio = boost::asio;

namespace seeder {

class DvrAppContext::Impl {
public:
    void init_args(int argc, char **argv) {
        auto cli = service_manager.add_service<CommandLineService>();
        cli->set_args(argc, argv);
    }

    void startup() {
        service_manager.add_service<ConfigService>();
        
        service_manager.add_service<HttpService>();
        service_manager.add_service<St2110Service>();
        service_manager.add_service<StatService>();
        service_manager.add_service<DelayControlService>();

        service_manager.start();
    }

    void run() {
        asio::post(io_ctx, [this] {
            startup();
        });

        auto guard = asio::make_work_guard(io_ctx);
        io_ctx.run();
    }

    ServiceManager service_manager;
    asio::io_context io_ctx;
};

DvrAppContext::DvrAppContext(): p_impl(new Impl) {}

DvrAppContext::~DvrAppContext() {}

void DvrAppContext::init_args(int argc, char **argv) {
    p_impl->init_args(argc, argv);
}

void DvrAppContext::run() {
    p_impl->run();
}

} // namespace seeder