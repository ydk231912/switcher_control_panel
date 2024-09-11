#include "program_var.h"

#include <mutex>
#include <cstdlib>

#include "util/fs.h"

namespace seeder {

class ProgramVarRegistry::Impl {
public:
    std::mutex mutex;
    nlohmann::json vars;
};

ProgramVarRegistry::ProgramVarRegistry(): p_impl(new Impl) {}

ProgramVarRegistry::~ProgramVarRegistry() {}

ProgramVarRegistry & ProgramVarRegistry::get_instance() {
    static ProgramVarRegistry instance;
    return instance;
}

void ProgramVarRegistry::add(const std::string &name, const nlohmann::json &value) {
    std::unique_lock<std::mutex> lock(p_impl->mutex);
    p_impl->vars[name] = value;
}

nlohmann::json ProgramVarRegistry::find(const std::string &name) {
    std::unique_lock<std::mutex> lock(p_impl->mutex);
    nlohmann::json r;
    auto iter = p_impl->vars.find(name);
    if (iter != p_impl->vars.end()) {
        r = *iter;
    }
    return r;
}

void ProgramVarRegistry::load_from_envfile(const std::string &env_name) {
    const char *env_value = getenv(env_name.c_str());
    if (!env_value) {
        return;
    }
    this->load_from_file(env_value);
}

void ProgramVarRegistry::load_from_file(const std::string &file_path) {
    if (file_path.empty()) {
        return;
    }
    try {
        std::string file_content;
        seeder::util::read_file(file_path, file_content);
        auto vars = nlohmann::json::parse(file_content);
        std::unique_lock<std::mutex> lock(p_impl->mutex);
        p_impl->vars.update(vars);
    } catch (nlohmann::json::exception &e) {
        return;
    }
}

} // namespace seeder