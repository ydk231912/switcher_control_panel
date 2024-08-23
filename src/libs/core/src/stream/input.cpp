#include "core/stream/input.h"

namespace seeder::core {

input::input(const std::string &_source_id): source_id(_source_id) {}

void input::start() {
    int expected = running_status::INIT;
    this->status.compare_exchange_strong(expected, running_status::STARTED);
}

void input::stop() {
    int current_status = status;
    if (current_status == running_status::STOPPED) {
        return;
    }
    if (status.compare_exchange_strong(current_status, running_status::STOPPED)) {
        do_stop();
    }
}

void input::do_stop() {}

bool input::is_started() const {
    return this->status == running_status::STARTED;
}

bool input::is_stopped() const {
    return this->status == running_status::STOPPED;
}

input::~input() {}

const std::string & input::get_source_id() const {
    return source_id;
}

} // namespace seeder::core