#include "ffmpeg_util.h"

extern "C" {
#include <libavutil/timecode.h>
}

namespace seeder {

namespace ffmpeg {

const std::string DEFAULT_ZERO_TIMECODE = "00:00:00:00";

std::string make_time_code(const boost::rational<int> &rate, int frame_num) {
    AVTimecode tc;
    av_timecode_init_from_string(&tc, av_make_q(rate.numerator(), rate.denominator()), DEFAULT_ZERO_TIMECODE.c_str(), NULL);
    char tcbuf[AV_TIMECODE_STR_SIZE] = "";
    av_timecode_make_string(&tc, tcbuf, frame_num);
    return tcbuf;
}

} // namespace seeder::ffmpeg

} // namespace seeder