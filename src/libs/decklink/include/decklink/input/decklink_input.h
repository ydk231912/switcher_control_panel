/**
 * @file decklink_input.h
 * @author 
 * @brief decklink stream input handle
 * @version 1.0
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <mutex>
#include <memory>
#include <deque>
#include <condition_variable>
#include <atomic>

#include "../interop/DeckLinkAPI.h"

#include "core/stream/input.h"
#include "core/video_format.h"
#include "core/util/buffer.h"


extern "C" {
  #include "libavutil/buffer.h"
}

namespace seeder::decklink
{
    struct decklink_input_stat {
      int frame_cnt = 0;
      int invalid_frame_cnt = 0;
    };

    struct decklink_audio_producer;

    class decklink_input : public seeder::core::input, public IDeckLinkInputCallback
    {
        friend struct decklink_audio_producer;
      public:
        /**
         * @brief Construct a new decklink input object.
         * initialize deckllink device and start input stream
         */
        explicit decklink_input(const std::string &source_id, int device_id, seeder::core::video_format_desc& format_desc, const std::string &pixel_format);
        ~decklink_input();


        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
        virtual ULONG STDMETHODCALLTYPE AddRef(void);
        virtual ULONG STDMETHODCALLTYPE  Release(void);

        /**
         * @brief change display mode and restart input streams
         * 
         * @param events 
         * @param mode 
         * @param format_flags 
         * @return HRESULT 
         */
        virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags) override;
        
        /**
         * @brief handle arrived frame from decklink card. 
         * push frame into the buffer, if the buffer is full, discard the old frame
         * 
         * @param video_frame 
         * @param audio_frame 
         * @return HRESULT 
         */
        virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*) override;

        /**
         * @brief start input stream handle
         * 
         */
        void start();
            
        /**
         * @brief stop input stream handle
         * 
         */
        void do_stop();

        /**
         * @brief push a frame into this input stream
         * 
         */
        void set_frame(std::shared_ptr<seeder::core::frame> frm);
        void set_video_frame(std::shared_ptr<AVFrame> vframe);
        void set_audio_frame(std::shared_ptr<AVFrame> aframe);


        /**
         * @brief Get a frame from this input stream
         * 
         */
        std::shared_ptr<seeder::core::frame> get_frame();
        std::shared_ptr<AVFrame> get_video_frame();
        std::shared_ptr<AVFrame> get_audio_frame();

        void set_audio_frame_slice(std::shared_ptr<seeder::core::buffer> asframe);
        void set_audio_frame_slice(const std::vector<std::shared_ptr<seeder::core::buffer>> &frames);
        std::shared_ptr<seeder::core::buffer> get_audio_frame_slice();

        decklink_input_stat get_stat();
        
        std::atomic<bool> has_signal = false;

      private:
        int decklink_index_;
        core::video_format_desc format_desc_;
        BMDVideoInputFlags video_flags_;
        BMDPixelFormat pixel_format_;
        BMDDisplayMode bmd_mode_;
        IDeckLinkDisplayMode* display_mode_;
        IDeckLink* decklink_;
        IDeckLinkInput*	input_;

        //audio
        _BMDAudioSampleType sample_type_;

        //buffer
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<seeder::core::frame>> frame_buffer_;
        const size_t frame_capacity_ = 3;
        std::shared_ptr<seeder::core::frame> last_frame_;
        std::condition_variable frame_cv_;

        // video buffer
        std::mutex vframe_mutex_;
        std::deque<std::shared_ptr<AVFrame>> vframe_buffer_;
        const size_t vframe_capacity_ = 6;
        std::shared_ptr<seeder::core::frame> last_vframe_;
        std::condition_variable vframe_cv_;
        std::atomic<int> receive_frame_stat = 0;
        std::atomic<int> invalid_frame_stat = 0;
        std::shared_ptr<AVBufferPool> av_buffer_pool;

        // audio buffer
        std::mutex aframe_mutex_;
        std::deque<std::shared_ptr<AVFrame>> aframe_buffer_;
        // const size_t aframe_capacity_ = 2;
        const size_t aframe_capacity_ = 40;
        std::shared_ptr<seeder::core::frame> last_aframe_;
        std::condition_variable aframe_cv_;
        std::unique_ptr<decklink_audio_producer> audio_producer;

        // audio slice buffer
        std::mutex asframe_mutex_;
        std::deque<std::shared_ptr<seeder::core::buffer>> asframe_buffer_;
        const size_t asframe_capacity_ = 1600; // 1 audio frame(p25:40ms) slice to 125us a frame, slices = 40ms / 0.125ms
        std::shared_ptr<seeder::core::buffer> last_asframe_;
        std::condition_variable asframe_cv_;


    };
}