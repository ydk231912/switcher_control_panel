/**
 * seeder frame
 *
 */
#pragma once

#include <memory>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

namespace seeder { namespace core {
    struct frame
    {
      std::shared_ptr<AVFrame> video;
      uint32_t timestamp;
    };
}}