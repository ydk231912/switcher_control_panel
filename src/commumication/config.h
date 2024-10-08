#pragma once 

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace seeder{

namespace config{

struct Config{
    // std::string pgm_name[100];
    // std::string pvw_name[100];

    std::vector<std::string> pgm_name;
    std::vector<std::string> pvw_name;
};

} // namespace config
} // namespace seeder