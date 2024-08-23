#include "service_manager.h"

#include <stdexcept>

#include "core/util/logger.h"

using seeder::core::logger;

namespace seeder {

void ServiceManager::add_service(std::shared_ptr<Service> service) {
    auto id = service->get_id();
    if (id.empty()) {
        logger->error("ServiceManager add_service empty id");
        throw std::runtime_error("add_service");
    }
    if (service_map.find(id) != service_map.end()) {
        logger->error("ServiceManager add_service duplicated id: {}", id);
        throw std::runtime_error("add_service");
    }
    service_map[id] = service;
    services.push_back(service);
}

void ServiceManager::start() {
    for (auto &service : services) {
        ServiceSetupCtx ctx(*this);
        service->setup(ctx);
    }
    for (auto &service : services) {
        logger->debug("ServiceManager start service {}", service->get_id());
        service->start();
    }
    logger->debug("ServiceManager start finish");
}

} // namespace seeder