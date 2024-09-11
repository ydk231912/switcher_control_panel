#pragma once

#include <string>

#include <boost/rational.hpp>

namespace seeder {

namespace ffmpeg {

extern const std::string DEFAULT_ZERO_TIMECODE;

std::string make_time_code(const boost::rational<int> &rate, int frame_num);

} // namespace seeder::ffmpeg

} // namespace seeder