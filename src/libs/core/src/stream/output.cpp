#include "core/stream/output.h"

namespace seeder::core {

output::output(const std::string &_output_id): output_id(_output_id) {}

void output::start() {
    int expected = running_status::INIT;
    this->status.compare_exchange_strong(expected, running_status::STARTED);
}

bool output::is_started() const {
    return this->status > running_status::INIT;
}

output::~output() {}

void output::dump_stat() {}

const std::string & output::get_output_id() const {
    return output_id;
}

} // namespace seeder::core
