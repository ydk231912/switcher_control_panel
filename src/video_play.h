#pragma once

#include <string>
#include <boost/lockfree/queue.hpp>
#include <mutex>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "util/frame.h"
#include "sdl_consumer.h"

namespace seeder
{
    class video_play
    {
      public:
        video_play();
        ~video_play();

        /**
         * @brief simple video play by ffmpeg and SDL
         * 
         * @param file_name: video path and file
         * @return int 
         */
        int play(std::string filename);
        int sdl_play(std::string filename);
        
        /**
         * @brief get video frame from the buffer
         * 
         * @return frame
         */
        util::frame receive_frame();

        // get system precision time
        int64_t get_timestamp();

      private:
        std::shared_ptr<AVFrame> frame_alloc();


      public:
        std::mutex buffer_mutex_;
        std::deque<util::frame> frame_buffer_;
        std::size_t buffer_capacity_ = 25;


        //test
    };
}