/**
 * @file ffmpeg_producer.h
 * @author 
 * @brief ffmpeg video player
 * @version 
 * @date 2022-06-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

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

#include "core/frame/frame.h"


namespace seeder::ffmpeg
{
    class ffmpeg_producer
    {
      public:
        ffmpeg_producer();
        ~ffmpeg_producer();

        /**
         * @brief simple video play by ffmpeg and SDL
         * 
         * @param file_name: video path and file
         * @return int 
         */
        void run(std::string filename);
        
        /**
         * @brief Get the frame object from the frame_buffer
         * 
         * @return std::shared_ptr<AVFrame> 
         */
        std::shared_ptr<AVFrame> get_frame();

      //buffer
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<AVFrame>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<AVFrame> last_frame_;
        std::condition_variable frame_cv_;
      

    };
} // namespace seeder::ffmpeg