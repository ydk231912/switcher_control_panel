/**
 * @file frame.h
 * @author 
 * @brief frame data struct
 * @version 1.0
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <memory>
#include <vector>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

namespace seeder::core
{
    /**
     * @brief frame data struct, include video, audio
     * 
     */
    class frame
    {
      public:
        frame();
        ~frame();

        std::shared_ptr<AVFrame> video;
        std::shared_ptr<AVFrame> audio;

        uint32_t timestamp;
        uint32_t frame_id = 1;


      private:
            
        
    };
}