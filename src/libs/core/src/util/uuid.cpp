#include "core/util/uuid.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace seeder::core {

    std::string generate_uuid() {
        return boost::uuids::to_string(boost::uuids::random_generator()());
    }

} // namespace seeder::core