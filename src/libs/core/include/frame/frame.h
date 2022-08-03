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
        void set_video_data(AVFrame* avframe);
        void set_audio_data(AVFrame* avframe);

        //std::vector<uint8_t *> video_data;
        uint8_t *video_data[AV_NUM_DATA_POINTERS];
        int linesize[AV_NUM_DATA_POINTERS];
        //std::vector<int> linesize;
        uint8_t *audio_data[AV_NUM_DATA_POINTERS];
        int format;
        int width, height;
        int nb_samples;
        int key_frame;
        int interlaced_frame, top_field_first;
        int64_t pts;
        uint32_t timestamp;

      private:
            
        
    };
}