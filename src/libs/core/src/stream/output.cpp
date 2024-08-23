#include "core/stream/output.h"

namespace seeder::core {

output::output(const std::string &_output_id): output_id(_output_id) {}

void output::start() {
    int expected = running_status::INIT;
    this->status.compare_exchange_strong(expected, running_status::STARTED);
}

bool output::is_started() const noexcept {
    return this->status > running_status::INIT;
}

void output::stop() {
    int current_status = status;
    if (current_status == running_status::STOPPED) {
        return;
    }
    if (status.compare_exchange_strong(current_status, running_status::STOPPED)) {
        do_stop();
    }
}

void output::do_stop() {}

bool output::is_stopped() const noexcept {
    return this->status == running_status::STOPPED;
}

output::~output() {}

const std::string & output::get_output_id() const {
    return output_id;
}

} // namespace seeder::core
