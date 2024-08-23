#include "st2110_service.h"

#include "core/util/logger.h"
#include "server/config_servce.h"
#include "st2110/st2110_session.h"

#include "st2110_config.h"


using seeder::core::logger;

namespace seeder {


class St2110Service::Impl {
public:
    void start() {
        session_manager.reset(new St2110SessionManager);
        try {
            auto st2110_config = config_proxy.try_get();
            if (st2110_config) {
                session_manager->init(st2110_config.value());
            }
        } catch (std::exception &e) {
            logger->error("app_ctx startup st2110_session_manager init error: {}", e.what());
        }
    }

    std::shared_ptr<ConfigService> config_service;
    ConfigCategoryProxy<seeder::config::ST2110Config> config_proxy;
    std::shared_ptr<St2110SessionManager> session_manager;
};


void St2110Service::setup(ServiceSetupCtx &ctx) {
    p_impl->config_service = ctx.get_service<ConfigService>();
    p_impl->config_proxy.init_add(*p_impl->config_service, CONFIG_NAME);
}

void St2110Service::start() {
    p_impl->start();
}

St2110Service::St2110Service(): p_impl(new Impl) {}

St2110Service::~St2110Service() {}

std::shared_ptr<St2110SessionManager> St2110Service::get_session_manager() {
    return p_impl->session_manager;
}

} // namespace seeder