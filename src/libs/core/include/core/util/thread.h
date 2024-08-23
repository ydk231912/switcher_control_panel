#pragma once

#include <string>
#include <functional>

namespace seeder::core::util {

void unbind_lcore();

void run_with_unbind_lcore(std::function<void()> f);

std::string get_thread_name();

void set_thread_name(const std::string &name);

/** 设置线程亲和性
参数 str , 支持:
    单个数字，例如 9
    范围，例如 2-6
    组合，例如 (0,2-4,6)
*/
void thread_set_affinity(const std::string &str);

void run_with_lcore(const std::string &str, std::function<void()> f);

} // namespace seeder::core::util