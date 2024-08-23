#pragma once

#include <core/util/buffer.h>
#include "core/video_format.h"

namespace seeder {

class St2110BaseOutput {
public:
    virtual ~St2110BaseOutput() {}

    // 阻塞
    virtual std::shared_ptr<seeder::core::AbstractBuffer> wait_get_video_frame() = 0;
    // 阻塞
    virtual std::shared_ptr<seeder::core::AbstractBuffer> wait_get_audio_frame_slice() = 0;
};

} // namespace seeder