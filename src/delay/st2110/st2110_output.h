#pragma once

#include <core/util/buffer.h>
#include "core/video_format.h"

namespace seeder {

class St2110BaseOutput {
public:
    virtual ~St2110BaseOutput() {}

    // 阻塞
    virtual bool wait_get_video_frame(void *dst) = 0;
    // 阻塞
    virtual bool wait_get_audio_frame(void *dst) = 0;
};

} // namespace seeder