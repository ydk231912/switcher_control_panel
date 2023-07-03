#pragma once

#include "app_base.h"

#include <boost/core/noncopyable.hpp>
#include <memory>

namespace seeder {

class st_http_server : boost::noncopyable {
public:
    explicit st_http_server(st_app_context *app_ctx);

    void start();
    void stop();

    ~st_http_server();
private:
    class impl;
    std::unique_ptr<st_http_server::impl> p_impl; 
};

} // namespace seeder