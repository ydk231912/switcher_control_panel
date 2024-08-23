#pragma once

#include <memory>

#include <nlohmann/json.hpp>

#include "st2110/st2110_source.h"


namespace seeder {

namespace config {

struct St2110DummyFileInputConfig {
    std::string file;
    std::string video_format;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        St2110DummyFileInputConfig,
        file,
        video_format
    )
};
    
} // namespace seeder::config

class St2110DummyFileInput {
public:
    explicit St2110DummyFileInput(const seeder::config::St2110DummyFileInputConfig &in_config);
    ~St2110DummyFileInput();

    void set_source(const std::shared_ptr<St2110BaseSource> &in_source);
    void start();

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder