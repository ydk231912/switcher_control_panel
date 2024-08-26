#pragma once

#include <memory>
#include <any>

#include "server/service.h"

#include <boost/signals2/signal.hpp>


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

    boost::signals2::signal<void(DeviceEventType, const std::any &)> & get_device_event_signal();

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder