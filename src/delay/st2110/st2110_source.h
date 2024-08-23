#pragma once

#include <core/util/buffer.h>

namespace seeder {

class St2110BaseSource {
public:
    virtual ~St2110BaseSource() {}

    virtual bool push_video_frame(seeder::core::AbstractBuffer &in_buffer) = 0;
    virtual void push_audio_frame(seeder::core::AbstractBuffer &in_buffer) = 0;
};
    
} // namespace seeder