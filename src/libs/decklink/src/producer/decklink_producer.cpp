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

#include <iostream>

#include "producer/decklink_producer.h"


namespace seeder::decklink {
    decklink_producer::decklink_producer()
    {
        HRESULT result;
        decklink_index_ = 1;
        pixel_format_ = bmdFormat8BitYUV;
        bmd_mode_ = bmdModeHD1080p50;
        
        // get the decklink device
        decklink_ = get_decklink(decklink_index_);
        if(decklink_ == nullptr)
        {
            std::cout << "Failed to get DeckLink." << std::endl;
            return;
        }

        // get input interface of the decklink device
        result = decklink_->QueryInterface(IID_IDeckLinkInput, (void**)&input_);
        if(result != S_OK)
        {
            std::cout << "Failed to get DeckLink input interface." << std::endl;
            return;
        }

        // get decklink input mode
        // result =  input_->GetDisplayMode(bmd_mode_, &display_mode_);
        // if(result != S_OK)
        // {
        //     std::cout << "Failed to get DeckLink input mode." << std::endl;
        //     return;
        // }

        // set the capture callback
        result = input_->SetCallback(this);
        if(result != S_OK)
        {
            std::cout << "Failed to set DeckLink input callback." << std::endl;
            return;
        }

        // enable input and start streams
        result = input_->EnableVideoInput(bmd_mode_, pixel_format_, 0);
        if(result != S_OK)
        {
            std::cout << "Failed to enable DeckLink video input." << std::endl;
            return;
        }

        // result = input_->EnableAudioInput();
        // if(result != S_OK)
        // {
        //     std::cout << "Enable DeckLink video input failed" << std::endl;
        //     return;
        // }

        result = input_->StartStreams();
        if(result != S_OK)
        {
            std::cout << "Failed to start DeckLink input streams." << std::endl;
            return;
        }

    }
    
    decklink_producer::~decklink_producer()
    {
        if(display_mode_ != nullptr)
            display_mode_->Release();

        if(input_ != nullptr)
        {
            input_->StopStreams();
            input_->DisableAudioInput();
            input_->DisableVideoInput();
            input_->Release();
        }

        if(decklink_ != nullptr)
            decklink_->Release();

    }


    ULONG decklink_producer::AddRef(void)
    {
        return 1;
    }

    ULONG decklink_producer::Release(void)
    {
        return 1;
    }

    HRESULT decklink_producer::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, 
                                                        IDeckLinkDisplayMode* mode, 
                                                        BMDDetectedVideoInputFormatFlags format_flags)
    {

        return S_OK;
    }

    HRESULT decklink_producer::VideoInputFrameArrived(IDeckLinkVideoInputFrame* video_frame, 
                                                       IDeckLinkAudioInputPacket* audio_frame)
    {

        return S_OK;
    }

    /**
     * @brief get decklink by selected device index
     * 
     * @param decklink_index 
     * @return IDeckLink* 
     */
    IDeckLink* decklink_producer::get_decklink(int decklink_index)
    {
        IDeckLink* decklink;
        IDeckLinkIterator* decklink_interator = CreateDeckLinkIteratorInstance();
        int i = 0;
        HRESULT result;
        if(!decklink_interator)
        {
            std::cout << "Get decklink failed,requires the DeckLink drivers intalled." << std::endl;
            return nullptr;
        }

        while((result = decklink_interator->Next(&decklink)) == S_OK)
        {
            if(i >= decklink_index)
                break;
                
            i++;
            decklink->Release();
        }

        decklink_interator->Release();

        if(result != S_OK)
            return nullptr;

        return decklink;
    }

}