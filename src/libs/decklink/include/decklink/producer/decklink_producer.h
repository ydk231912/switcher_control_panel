/**
 * @file decklink_producer.h
 * @author 
 * @brief decklink producer, receive SDI frame from decklink card
 * @version 
 * @date 2022-05-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <mutex>
#include <memory>
#include <deque>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "../interop/DeckLinkAPI.h"
#include "core/video_format.h"

namespace seeder::decklink {
    class decklink_producer : public IDeckLinkInputCallback
    {
      public:
        /**
         * @brief Construct a new decklink producer::decklink producer object.
         * initialize deckllink device and start input stream
         */
        decklink_producer(int device_id, core::video_format_desc& format_desc);
        ~decklink_producer();


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
         * @brief start decklink capture
         * 
         */
        void start();

        /**
         * @brief stop decklink capture
         * 
         */
        void stop();

        /**
         * @brief Get the frame object from the frame_buffer
         * 
         * @return std::shared_ptr<AVFrame> 
         */
        std::shared_ptr<AVFrame> get_frame();

      private:
        int decklink_index_;
        core::video_format_desc format_desc_;
        BMDVideoInputFlags video_flags_;
        BMDPixelFormat pixel_format_;
        BMDDisplayMode bmd_mode_;
        IDeckLinkDisplayMode* display_mode_;
        IDeckLink* decklink_;
        IDeckLinkInput*	input_;

        //buffer
        std::mutex frame_mutex_;
        std::deque<std::shared_ptr<AVFrame>> frame_buffer_;
        const size_t frame_capacity_ = 5;
        std::shared_ptr<AVFrame> last_frame_;
    };

}