#pragma once

#include <chrono>

namespace seeder {

namespace core {

template<class D, class Clock = std::chrono::system_clock>
typename D::rep now_since_epoch() {
    return std::chrono::duration_cast<D>(Clock::now().time_since_epoch()).count();
}

} // namespace seeder::core

} // namespace seeder