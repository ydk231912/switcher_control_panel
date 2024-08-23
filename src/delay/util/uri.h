#pragma once

#include <string>
#include <memory>

namespace seeder {


class UriParser {
public:
    explicit UriParser(const std::string &s);
    ~UriParser();

    static std::string peek_scheme(const std::string &s);

    std::string get_scheme();
    std::string get_path();
    std::string get_query_param(const std::string &key);

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};


} // namespace seeder