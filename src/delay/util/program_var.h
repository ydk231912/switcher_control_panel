#pragma once

#include <memory>
#include <stdexcept>

#include <nlohmann/json.hpp>


namespace seeder {

class ProgramVarRegistry {
public:
    ~ProgramVarRegistry();

    static ProgramVarRegistry & get_instance();

    void add(const std::string &name, const nlohmann::json &value);

    nlohmann::json find(const std::string &name);

    void load_from_envfile(const std::string &env_name);
    void load_from_file(const std::string &file_path);

private:
    explicit ProgramVarRegistry();

    class Impl;
    std::unique_ptr<Impl> p_impl;
};

template<class T>
T get_program_var(const std::string &name, const T &default_value) noexcept {
    auto j = ProgramVarRegistry::get_instance().find(name);
    try {
        return j.get<T>();
    } catch (nlohmann::json::exception &) {}
    return default_value;
}

} // namespace seeder