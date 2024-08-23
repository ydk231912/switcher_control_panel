#pragma once

#include "server/service.h"


namespace seeder {

class St2110SessionManager;

class St2110Service : public Service {
public:
    explicit St2110Service();
    ~St2110Service();

    std::shared_ptr<St2110SessionManager> get_session_manager();

    SEEDER_SERVICE_STATIC_ID("st2110");

    void setup(ServiceSetupCtx &ctx) override;

    void start() override;

    static inline const char *CONFIG_NAME = "st2110";

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder