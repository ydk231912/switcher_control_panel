#pragma once

#include <memory>

#include "server/service.h"

namespace seeder {

class StatService : public Service {
public:
    explicit StatService();
    ~StatService();

    SEEDER_SERVICE_STATIC_ID("stat");

    void setup(ServiceSetupCtx &ctx) override;

    void start() override;

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder