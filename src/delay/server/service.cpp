#include "service.h"

#include "service_manager.h"

namespace seeder {

std::shared_ptr<Service> ServiceSetupCtx::find_service(const std::string &id) {
    auto iter = service_manager.service_map.find(id);
    if (iter == service_manager.service_map.end()) {
        return nullptr;
    }
    return iter->second;
}

} // namespace seeder