#pragma once

#include <stdexcept>

namespace seeder {

namespace config {

struct ST2110InputConfig;
struct ST2110OutputConfig;

class ValidationException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

void validate_input(const seeder::config::ST2110InputConfig &input);

void validate_output(const seeder::config::ST2110OutputConfig &output);

struct ConfigCompareResult {
    bool format_changed = false;  // 需要重新实例化相关对象
    bool address_changed = false;  // 不用重新实例化，可以直接更新原对象
    bool name_changed = false;  // 只需更新多画面等参数

    bool has_any_changed() const {
        return format_changed || address_changed || name_changed;
    }

    void merge(const ConfigCompareResult &other) {
        format_changed = format_changed || other.format_changed;
        address_changed = address_changed || other.address_changed;
        name_changed = name_changed || other.name_changed;
    }
};

ConfigCompareResult compare_input(
    const seeder::config::ST2110InputConfig &old_value,
    const seeder::config::ST2110InputConfig &new_value);

ConfigCompareResult compare_output(
    const seeder::config::ST2110OutputConfig &old_value,
    const seeder::config::ST2110OutputConfig &new_value);

}  // namespace config

}  // namespace seeder