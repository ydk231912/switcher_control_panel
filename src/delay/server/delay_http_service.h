#pragma once

#include "server/service.h"


namespace seeder {

class DelayHttpService : public Service {
public:
    explicit DelayHttpService();
    ~DelayHttpService();

    SEEDER_SERVICE_STATIC_ID("delay_http")

    void setup(ServiceSetupCtx &ctx) override;

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder