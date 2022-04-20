/**
 * @file decklink_producer.h
 * @author your name (you@domain.com)
 * @brief decklink producer, capture SDI frame by decklink card
 * @version 0.1
 * @date 2022-04-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "../interop/DeckLinkAPI.h"

namespace seeder::decklink {
    class decklink_producer : public IDeckLinkInputCallback
    {
      public:
        decklink_producer();
        ~decklink_producer();


        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
        virtual ULONG STDMETHODCALLTYPE AddRef(void);
        virtual ULONG STDMETHODCALLTYPE  Release(void);
        virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags) override;
        virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*) override;

        IDeckLink* get_decklink(int);


      private:
        int decklink_index_;
        BMDPixelFormat pixel_format_;
        BMDDisplayMode bmd_mode_;
        IDeckLinkDisplayMode* display_mode_;
        IDeckLink* decklink_;
        IDeckLinkInput*	input_;

    };

}