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

#include "../interop/DeckLinkAPI.h"

#include "core/stream/input.h"
#include "core/video_format.h"

using namespace seeder::core;
namespace seeder::decklink
{
    class decklink_input : public input, public IDeckLinkInputCallback
    {
      public:
        /**
         * @brief Construct a new decklink input object.
         * initialize deckllink device and start input stream
         */
        decklink_input(int device_id, video_format_desc& format_desc);
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
        void stop();

        /**
         * @brief push a frame into this input stream
         * 
         */
        void set_frame(std::shared_ptr<frame> frm);

        /**
         * @brief Get a frame from this input stream
         * 
         */
        std::shared_ptr<frame> get_frame();

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
        std::deque<std::shared_ptr<frame>> frame_buffer_;
        const size_t frame_capacity_ = 3;
        std::shared_ptr<frame> last_frame_;

    };
}