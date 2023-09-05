/**
 * @file decklink_output.h
 * @author
 * @brief decklink output, send SDI frame by decklink card
 * @version 1.0
 * @date 2022-07-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once
#include <mutex>
#include <memory>
#include <deque>
#include <condition_variable>
#include <thread>
#include <atomic>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "../interop/DeckLinkAPI.h"
#include "core/video_format.h"
#include "core/stream/output.h"

namespace seeder::decklink
{
    struct decklink_output_stat {
      int frame_cnt = 0;  
    };

    class decklink_output : public core::output
    {
      public:
        decklink_output(const std::string &output_id, int device_id, const core::video_format_desc &format_desc, const std::string &pixel_format);
        ~decklink_output();

        /**
         * @brief start output stream handle
         * 
         */
        void start();
            
        void consume_st_video_frame(void *frame, uint32_t width, uint32_t height);
        void consume_st_audio_frame(void *frame, size_t frame_size);

        /**
         * @brief push a frame into this output stream buffer
         * 
         */
        void set_frame(std::shared_ptr<core::frame> frm);
        void set_video_frame(std::shared_ptr<AVFrame> vframe);
        void set_audio_frame(std::shared_ptr<AVFrame> aframe);

        /**
         * @brief Get a frame from this output stream buffer
         * 
         */
        std::shared_ptr<core::frame> get_frame();
        std::shared_ptr<AVFrame> get_video_frame();
        std::shared_ptr<AVFrame> get_audio_frame();
        /**
         * @brief push a video frame into this output stream
         * 
         */
        void display_video_frame();

        /**
         * @brief push a audio frame into this output stream
         * 
         */
        void display_audio_frame();

        decklink_output_stat get_stat();

      protected:
        /**
         * @brief stop output stream handle
         * 
         */
        void do_stop();

      private:
        int decklink_index_;
        core::video_format_desc format_desc_;
        BMDVideoInputFlags video_flags_;
        BMDPixelFormat pixel_format_ = bmdFormat10BitYUV;//bmdFormat8BitBGRA;bmdFormat10BitYUV
        std::string pixel_format_string_;
        BMDDisplayMode bmd_mode_;
        IDeckLinkDisplayMode* display_mode_;
        IDeckLink* decklink_;
        IDeckLinkOutput* output_;
        std::atomic<bool> abort_ = { false };
        std::atomic<int> display_frame_count = 0;
        int frame_convert_count = 0;
        uint64_t display_video_frame_us_sum = 0;
        uint64_t display_audio_frame_us_sum = 0;
        uint64_t memcpy_us_sum = 0;

        IDeckLinkMutableVideoFrame*	playbackFrame_ = nullptr;
        IDeckLinkMutableVideoFrame*	yuv10Frame_ = nullptr;
        IDeckLinkVideoConversion* frameConverter = nullptr;
        SwsContext* sws_ctx_ = nullptr;
        AVFrame* dst_frame_ = nullptr;
        uint8_t * dst_frame_buffer_ = nullptr;
        // frame buffer pointer
        uint8_t* vframe_buffer = nullptr;
        uint8_t* aframe_buffer = nullptr;
        // audio
        _BMDAudioSampleType sample_type_;

        //buffer
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<core::frame>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<core::frame> last_frame_;
        std::condition_variable frame_cv_;

        std::unique_ptr<std::thread> decklink_thread_;

        // video buffer
        std::mutex vframe_mutex_;
        std::deque<std::shared_ptr<AVFrame>> vframe_buffer_;
        const size_t vframe_capacity_ = 5;
        std::shared_ptr<seeder::core::frame> last_vframe_;
        std::condition_variable vframe_cv_;
        bool interlace_frame_ready = false;
        std::unique_ptr<uint8_t[]> half_height_buffer;

        // audio buffer
        std::mutex aframe_mutex_;
        std::deque<std::shared_ptr<AVFrame>> aframe_buffer_;
        const size_t aframe_capacity_ = 5;
        std::shared_ptr<seeder::core::frame> last_aframe_;
        std::condition_variable aframe_cv_;

    };

    /*
    class decklink_async_output : public core::output
    {
    public:
      explicit decklink_async_output(const std::string &output_id, int device_id, const core::video_format_desc &format_desc, const std::string &pixel_format);
      ~decklink_async_output();

      void start();

      void consume_st_video_frame(void *frame, uint32_t width, uint32_t height);
      void consume_st_audio_frame(void *frame, size_t frame_size);

      void display_video_frame();
      void display_audio_frame();

      void dump_stat();

      void set_frame(std::shared_ptr<core::frame> frm);
      void set_video_frame(std::shared_ptr<AVFrame> vframe);
      void set_audio_frame(std::shared_ptr<AVFrame> aframe);
      std::shared_ptr<core::frame> get_frame();
      std::shared_ptr<AVFrame> get_video_frame();
      std::shared_ptr<AVFrame> get_audio_frame();

    protected:
      void do_stop();

    private:
      class impl;
      friend class impl;
      friend class PlayoutDelegate;
      std::unique_ptr<impl> p_impl;
    };
    */
}