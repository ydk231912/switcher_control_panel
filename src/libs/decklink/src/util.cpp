/**
 * @file util.h
 * @author 
 * @brief decklink related tools method
 * @version 
 * @date 2022-05-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "util.h"

namespace seeder::decklink::util
{
    /**
     * @brief get decklink by selected device index
     * 
     * @param decklink_index 
     * @return IDeckLink* 
     */
    IDeckLink* get_decklink(int decklink_index)
    {
        IDeckLink* decklink;
        IDeckLinkIterator* decklink_interator = CreateDeckLinkIteratorInstance();
        int i = 0;
        HRESULT result;
        if(!decklink_interator)
        {
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

} // namespace seeder::decklink