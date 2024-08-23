#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <atomic>

#include <spdlog/common.h>
#include "core/util/logger.h"

namespace seeder {

class AbstractStatRecorder {
public:
    virtual ~AbstractStatRecorder() = default;
    virtual void dump() = 0;
    virtual void reset() = 0;
};

template<class T>
class SimpleValueStatRecorder : public seeder::AbstractStatRecorder {
public:
    explicit SimpleValueStatRecorder(spdlog::level::level_enum _lvl, const std::string &_name): log_level(_lvl), name(_name) {}

    template<class F>
    explicit SimpleValueStatRecorder(spdlog::level::level_enum _lvl, const std::string &_name, F &&f): log_level(_lvl), name(_name), dump_condition(std::forward<F>(f)) {}

    T get_value() {
        return value.load();
    }

    std::atomic<T> & get_atomic() {
        return value;
    }

    void dump() override {
        auto current_value = get_value();
        if (dump_condition && !dump_condition(current_value)) {
            return;
        }
        seeder::core::logger->log(log_level, "{}={}", name, current_value);
    }

    void reset() override {
        T default_value = {};
        value = default_value;
    }

    static bool dump_condition_no_zero(T v) {
        return v;
    }

private:
    spdlog::level::level_enum log_level;
    std::string name;
    std::atomic<T> value = {};
    std::function<bool(T)> dump_condition;
};

class StatRecorderRegistry {
public:
    static StatRecorderRegistry & get_instance() {
        static StatRecorderRegistry instance;
        return instance;
    }

    void dump() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto iter = recorder_list.begin(); iter != recorder_list.end();) {
            auto recorder = iter->lock();
            if (recorder) {
                recorder->dump();
                recorder->reset();
                ++iter;
            } else {
                recorder_list.erase(iter);
            }
        }
    }

    void add_recorder(const std::shared_ptr<AbstractStatRecorder> &recorder) {
        std::weak_ptr<AbstractStatRecorder> weak_ptr(recorder);
        std::lock_guard<std::mutex> lock(mutex);
        recorder_list.push_back(weak_ptr);
    }

    template<class T>
    static const std::shared_ptr<T> & with_recorder(const std::shared_ptr<T> &recorder) {
        get_instance().add_recorder(recorder);
        return recorder;
    }

private:
    explicit StatRecorderRegistry() {}
    explicit StatRecorderRegistry(const StatRecorderRegistry &) = delete;
    explicit StatRecorderRegistry(StatRecorderRegistry &&) = delete;
    StatRecorderRegistry & operator=(const StatRecorderRegistry &) = delete;
    StatRecorderRegistry & operator=(StatRecorderRegistry &&) = delete;

    std::mutex mutex;
    std::list<std::weak_ptr<AbstractStatRecorder>> recorder_list;
};

} // namespace seeder


#define SEEDER_STATIC_STAT_RECORDER(ty, var_name, log_level, ...) static std::shared_ptr<ty> var_name = seeder::StatRecorderRegistry::with_recorder(std::make_shared<ty>(spdlog::level::level_enum::log_level, __VA_ARGS__))
#define SEEDER_STATIC_STAT_RECORDER_SIMPLE_VALUE(ty, ...) SEEDER_STATIC_STAT_RECORDER(seeder::SimpleValueStatRecorder<ty>, __VA_ARGS__)
#define SEEDER_STATIC_STAT_RECORDER_SIMPLE_VALUE_NO_ZERO(ty, ...) SEEDER_STATIC_STAT_RECORDER(seeder::SimpleValueStatRecorder<ty>, __VA_ARGS__, &seeder::SimpleValueStatRecorder<ty>::dump_condition_no_zero);