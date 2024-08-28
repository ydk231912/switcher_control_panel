#pragma once

#include <memory>
#include <any>
#include <future>

#include <boost/signals2/signal.hpp>
#include <nlohmann/json.hpp>

#include "server/service.h"


namespace seeder {

enum class DeviceEventType {
    Added,
    Updated,
    Removed,
};

class DelayControlService : public Service {
public:
    explicit DelayControlService();
    ~DelayControlService();

    SEEDER_SERVICE_STATIC_ID("delay_control");

    void setup(ServiceSetupCtx &ctx) override;

    void start() override;

    std::future<nlohmann::json> get_device_info_async();

    std::future<void> update_device_config_async(const nlohmann::json &param);

    boost::signals2::signal<void(DeviceEventType, const std::any &)> & get_device_event_signal();

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder