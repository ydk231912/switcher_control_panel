#pragma once

#include <unordered_map>
#include <list>

#include "server/service.h"

namespace seeder {

class ServiceManager {
    friend class ServiceSetupCtx;
public:
    template<class T>
    std::shared_ptr<T> add_service() {
        std::shared_ptr<T> service(new T);
        add_service(service);
        return service;
    }

    void add_service(std::shared_ptr<Service> service);

    void start();    

private:
    std::list<std::shared_ptr<Service>> services;
    std::unordered_map<std::string, std::shared_ptr<Service>> service_map;
};

} // namespace seeder