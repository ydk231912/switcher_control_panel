#pragma once

#include <memory>
#include <string>

namespace seeder {

class Service;
class ServiceManager;

class ServiceSetupCtx {
public:
    explicit ServiceSetupCtx(ServiceManager &in_service_manager): service_manager(in_service_manager) {}

    std::shared_ptr<Service> find_service(const std::string &id);

    template<class T>
    std::shared_ptr<T> get_service() {
        auto service = find_service(T::ID);
        return std::dynamic_pointer_cast<T>(service);
    }

    template<class T>
    void get_service(std::shared_ptr<T> &out_service) {
        out_service = std::dynamic_pointer_cast<T>(find_service(T::ID));
    }

private:
    ServiceManager &service_manager;
};

class Service {
public:
    virtual ~Service() {}
    virtual void setup(ServiceSetupCtx &ctx) {}
    virtual void start() {}
    virtual std::string get_id() const = 0;
};

} // namespace seeder

#define SEEDER_SERVICE_STATIC_ID(id) \
    inline static const char *ID = id; \
    std::string get_id() const override { return ID; }
