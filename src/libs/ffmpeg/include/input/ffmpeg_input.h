/**
 * @file ffmpeg_output.h
 * @author 
 * @brief ffmpeg video player
 * @version 1.0
 * @date 2022-07-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <string>
#include <mutex>
#include <deque>
#include <condition_variable>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "core/stream/input.h"

namespace seeder::ffmpeg
{
    class ffmpeg_input : public core::input
    {
      public:
        ffmpeg_input(std::string filename);
        ~ffmpeg_input();

        /**
         * @brief start output stream handle
         * 
         */
        void start();
            
        /**
         * @brief stop output stream handle
         * 
         */
        void stop();

        /**
         * @brief push a frame into this output stream
         * 
         */
        void set_frame(std::shared_ptr<core::frame> frm);

        /**
         * @brief Get a frame from this output stream
         * 
         */
        std::shared_ptr<core::frame> get_frame();

        void set_video_data(std::shared_ptr<core::frame> frm, AVFrame* avframe);

      private:
        bool abort = false;
        std::string filename_;
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<core::frame>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<core::frame> last_frame_;
        std::condition_variable frame_cv_;

    };
}