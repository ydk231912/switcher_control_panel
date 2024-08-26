#pragma once

#include <memory>
#include <functional>
#include <any>
#include <future>
#include <optional>
#include <thread>

#include <nlohmann/json.hpp>

#include "server/service.h"


namespace seeder {

class ConfigCategory {
public:
    virtual ~ConfigCategory() {}

    void set_json_serializer(std::function<void(nlohmann::json &, const std::any &)> in_json_serializer) {
        json_serializer = in_json_serializer;
    }

    void set_json_deserializer(std::function<void(const nlohmann::json &, std::any &)> in_json_deserializer) {
        json_deserializer = in_json_deserializer;
    }

    virtual std::future<std::any> get_async() = 0;

    virtual std::future<void> update_async(const std::any &in_value) = 0;

    virtual void on_update(std::function<void(const std::any &)> handler) = 0;

    virtual void ensure_thread_can_async() = 0;

protected:
    std::function<void(nlohmann::json &, const std::any &)> json_serializer;
    std::function<void(const nlohmann::json &, std::any &)> json_deserializer;
};

class ConfigService : public Service {
    friend class ConfigCategoryImpl;
public:
    explicit ConfigService();
    ~ConfigService();

    SEEDER_SERVICE_STATIC_ID("config");

    void setup(ServiceSetupCtx &ctx) override;

    void start() override;

    template<class T>
    std::shared_ptr<ConfigCategory> add_category(const std::string &name) {
        auto category = add_category_impl(name);
        category->set_json_serializer([] (nlohmann::json &j, const std::any &any_value) {
            const T *ptr = std::any_cast<T>(&any_value);
            if (ptr) {
                j = *ptr;
            }
        });
        category->set_json_deserializer([] (const nlohmann::json &j, std::any &any_value) {
            T value = j;
            any_value = value;
        });
        return category;
    }

    std::shared_ptr<ConfigCategory> get_category(const std::string &name);

private:
    std::shared_ptr<ConfigCategory> add_category_impl(const std::string &name);

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};


template<class T>
class ConfigCategoryProxy {
public:
    void init(const std::shared_ptr<ConfigCategory> &in_config_category) {
        inner = in_config_category;
    }

    void init_add(ConfigService &service, const std::string &name) {
        auto category = service.add_category<T>(name);
        this->init(category);
    }

    void init_get(ConfigService &service, const std::string &name) {
        auto category = service.get_category(name);
        this->init(category);
    }

    void try_get_to(T &out_value) {
        try {
            out_value = this->get();
        } catch (std::bad_any_cast &) {}
    }

    std::optional<T> try_get() {
        try {
            return this->get();
        } catch (std::bad_any_cast &) {}
        return std::nullopt;
    }

    T get() {
        inner->ensure_thread_can_async();
        auto future = inner->get_async();
        return std::any_cast<T>(future.get());
    }

    void update(const T &in_value) {
        std::any any_value = in_value;
        inner->update_async(any_value).wait();
    }

    template<class F>
    void on_update(F &&handler) {
        inner->on_update(std::forward<F>(handler));
    }

private:
    std::shared_ptr<ConfigCategory> inner;
};


} // namespace seeder