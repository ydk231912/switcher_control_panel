#include "config_service.h"

#include <fstream>
#include <future>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <boost/asio.hpp>

#include <boost/signals2.hpp>

#include "server/command_line_service.h"
#include "core/util/logger.h"
#include "util/io.h"
#include "util/json_util.h"
#include "app_build_config.h"
#include "util/program_var.h"

namespace asio = boost::asio;

using seeder::core::logger;

namespace seeder {

namespace {

struct LogConfig {
    std::string level = "info";
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    LogConfig,
    level
)

} // namespace seeder::{}

class ConfigCategoryImpl : public ConfigCategory {
public:

    std::future<std::any> get_async() override;

    std::future<void> update_async(const std::any &in_value) override;

    void on_update(std::function<void(const std::any &)> handler) override {
        on_update_handler.connect(handler);
    }

    void ensure_thread_can_async() override;

    void to_json(nlohmann::json &j) {
        json_serializer(j, config_any_value);
    }

    void from_json(const nlohmann::json &j) {
        json_deserializer(j, config_any_value);
    }

public:
    std::shared_ptr<asio::io_context> io_ctx;
    ConfigService::Impl *service_impl = nullptr;
    std::string name;
    std::any config_any_value;
    boost::signals2::signal<void(const std::any &)> on_update_handler;
};

class ConfigService::Impl {
public:
    void load_config() {
        {
            std::ifstream f(config_file_path);
            if (!f) {
                logger->error("ConfigService load_config failed to open file: {}", config_file_path);
                return;
            }
            config_json = nlohmann::json::parse(f);
        }
        for (auto &[name, category] : categories) {
            nlohmann::json &category_json = config_json[name];
            try {
                category->from_json(category_json);
            } catch (std::exception &e) {
                logger->warn("ConfigService load_config category {} from_json error: {}", name, e.what());
            }
        }
    }

    void startup() {
        /* namespace fs = boost::filesystem;
        if (config.media_dir.empty() || !fs::is_directory(config.media_dir)) {
            logger->error("media_dir={} does not exists", config.media_dir);
            throw std::invalid_argument("media_dir");
        } */
        join_handle = seeder::async_run_with_new_io_context();
        for (auto &p : categories) {
            p.second->io_ctx = join_handle.get_io_ctx();
        }
    }

    void save_config() {
        nlohmann::json new_config_json = config_json;
        
        for (auto &[name, category] : categories) {
            nlohmann::json &category_json = new_config_json[name];
            try {
                category->to_json(category_json);
            } catch (std::exception &e) {
                logger->error("ConfigService save_config category {} to_json error: {}", name, e.what());
            }
        }

        std::ofstream f(config_file_path);
        if (!f) {
            logger->error("ConfigService save_config failed to open file: {}", config_file_path);
            return;
        }
        f << new_config_json.dump();
        config_json = new_config_json;

        logger->info("config save to {}", config_file_path);
    }

    void setup_log() {
        LogConfig log_config;
        seeder::json::object_safe_get_to(config_json, "log", log_config);

        std::vector<spdlog::sink_ptr> sinks;
        auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        sinks.push_back(console_sink);

        seeder::core::logger_init::setup_sinks(std::move(sinks));
        seeder::core::logger_init::setup_levels(log_config.level);
        seeder::core::logger = seeder::core::get_logger("SEEDER ST2110");
    }

    std::shared_ptr<ConfigCategory> add_category(const std::string &name) {
        if (categories.find(name) != categories.end()) {
            logger->error("ConfigSerivce add_category duplicated name: {}", name);
            throw std::runtime_error("add_category");
        }
        std::shared_ptr<ConfigCategoryImpl> category(new ConfigCategoryImpl);
        category->name = name;
        category->service_impl = this;
        categories[name] = category;
        return category;
    }

    std::shared_ptr<ConfigCategory> get_category(const std::string &name) const {
        auto iter = categories.find(name);
        if (iter == categories.end()) {
            return nullptr;
        }
        return iter->second;
    }

    std::string config_file_path;
    nlohmann::json config_json;
    std::unordered_map<std::string, std::shared_ptr<ConfigCategoryImpl>> categories;
    AsioJoinHandle join_handle;
};

ConfigService::ConfigService(): p_impl(new Impl) {}

ConfigService::~ConfigService() {}

void ConfigService::setup(ServiceSetupCtx &ctx) {
    auto cli = ctx.get_service<CommandLineService>();
    cli->add_option("config_file", p_impl->config_file_path);
    ProgramVarRegistry::get_instance().load_from_envfile("SEEDER_PROG_VAR_FILE");
    ProgramVarRegistry::get_instance().load_from_file("program_var.json");
}

void ConfigService::start() {
    p_impl->startup();
    p_impl->load_config();
    p_impl->setup_log();
    logger->debug("ConfigService finish");
    logger->info("{}", "rev: " PROJECT_GIT_HASH);
}

std::shared_ptr<ConfigCategory> ConfigService::add_category_impl(const std::string &name) {
    return p_impl->add_category(name);
}

std::shared_ptr<ConfigCategory> ConfigService::get_category(const std::string &name) {
    return p_impl->get_category(name);
}

std::future<std::any> ConfigCategoryImpl::get_async() {
    auto promise = std::make_shared<std::promise<std::any>>();
    auto future = promise->get_future();

    asio::post(*io_ctx, [this, promise] ()  {
        promise->set_value(config_any_value);
    });

    return future;
}

std::future<void> ConfigCategoryImpl::update_async(const std::any &in_value) {
    logger->debug("ConfigService category update: {}", name);
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    asio::post(*io_ctx, [this, promise, in_value] ()  {
        config_any_value = in_value;
        promise->set_value();
        try {
            on_update_handler(in_value);
        } catch (std::exception &e) {
            logger->error("ConfigCategoryImpl {} on_update error: {}", name, e.what());
        }
        service_impl->save_config();
    });

    return future;
}

void ConfigCategoryImpl::ensure_thread_can_async() {
    if (service_impl->join_handle.get_thread_id() == std::this_thread::get_id()) {
        logger->error("ConfigCategoryImpl thread cannot async!");
        throw std::runtime_error("ConfigCategoryImpl::ensure_thread_can_async");
    }
}

} // namespace seeder