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
#pragma once

#include <string>
#include <memory>

#include "interop/DeckLinkAPI.h"
#include "core/video_format.h"


[[maybe_unused]]
inline static void intrusive_ptr_add_ref(IUnknown *p) {
    p->AddRef();
}

[[maybe_unused]]
inline static void intrusive_ptr_release(IUnknown *p) {
    p->Release();
}

namespace seeder::decklink::util
{
    /**
     * @brief Get the decklink video format object
     * 
     * @param fmt 
     * @return BMDDisplayMode 
     */
    [[maybe_unused]]
    static BMDDisplayMode get_decklink_video_format(core::video_fmt fmt)
    {
        switch (fmt) {
            case core::video_fmt::pal:
                return bmdModePAL;
            case core::video_fmt::ntsc:
                return bmdModeNTSC;
            case core::video_fmt::x576p2500:
                return (BMDDisplayMode)ULONG_MAX;
            case core::video_fmt::x720p2398:
                return (BMDDisplayMode)ULONG_MAX;
            case core::video_fmt::x720p2400:
                return (BMDDisplayMode)ULONG_MAX;
            case core::video_fmt::x720p2500:
                return (BMDDisplayMode)ULONG_MAX;
            case core::video_fmt::x720p5000:
                return bmdModeHD720p50;
            case core::video_fmt::x720p2997:
                return (BMDDisplayMode)ULONG_MAX;
            case core::video_fmt::x720p5994:
                return bmdModeHD720p5994;
            case core::video_fmt::x720p3000:
                return (BMDDisplayMode)ULONG_MAX;
            case core::video_fmt::x720p6000:
                return bmdModeHD720p60;
            case core::video_fmt::x1080p2398:
                return bmdModeHD1080p2398;
            case core::video_fmt::x1080p2400:
                return bmdModeHD1080p24;
            case core::video_fmt::x1080i5000:
                return bmdModeHD1080i50;
            case core::video_fmt::x1080i5994:
                return bmdModeHD1080i5994;
            case core::video_fmt::x1080i6000:
                return bmdModeHD1080i6000;
            case core::video_fmt::x1080p2500:
                return bmdModeHD1080p25;
            case core::video_fmt::x1080p2997:
                return bmdModeHD1080p2997;
            case core::video_fmt::x1080p3000:
                return bmdModeHD1080p30;
            case core::video_fmt::x1080p5000:
                return bmdModeHD1080p50;
            case core::video_fmt::x1080p5994:
                return bmdModeHD1080p5994;
            case core::video_fmt::x1080p6000:
                return bmdModeHD1080p6000;
            case core::video_fmt::x1556p2398:
                return bmdMode2k2398;
            case core::video_fmt::x1556p2400:
                return bmdMode2k24;
            case core::video_fmt::x1556p2500:
                return bmdMode2k25;
            case core::video_fmt::x2160p2398:
                return bmdMode4K2160p2398;
            case core::video_fmt::x2160p2400:
                return bmdMode4K2160p24;
            case core::video_fmt::x2160p2500:
                return bmdMode4K2160p25;
            case core::video_fmt::x2160p2997:
                return bmdMode4K2160p2997;
            case core::video_fmt::x2160p3000:
                return bmdMode4K2160p30;
            case core::video_fmt::x2160p5000:
                return bmdMode4K2160p50;
            case core::video_fmt::x2160p5994:
                return bmdMode4K2160p5994;
            case core::video_fmt::x2160p6000:
                return bmdMode4K2160p60;
            default:
                return (BMDDisplayMode)ULONG_MAX;
        }
    }

    /**
     * @brief Get the seeder video format object
     * 
     * @param fmt 
     * @return core::video_format 
     */
    [[maybe_unused]]
    static core::video_fmt get_seeder_video_format(BMDDisplayMode fmt)
    {
        switch (fmt) {
            case bmdModePAL:
                return core::video_fmt::pal;
            case bmdModeNTSC:
                return core::video_fmt::ntsc;
            case bmdModeHD720p50:
                return core::video_fmt::x720p5000;
            case bmdModeHD720p5994:
                return core::video_fmt::x720p5994;
            case bmdModeHD720p60:
                return core::video_fmt::x720p6000;
            case bmdModeHD1080p2398:
                return core::video_fmt::x1080p2398;
            case bmdModeHD1080p24:
                return core::video_fmt::x1080p2400;
            case bmdModeHD1080i50:
                return core::video_fmt::x1080i5000;
            case bmdModeHD1080i5994:
                return core::video_fmt::x1080i5994;
            case bmdModeHD1080i6000:
                return core::video_fmt::x1080i6000;
            case bmdModeHD1080p25:
                return core::video_fmt::x1080p2500;
            case bmdModeHD1080p2997:
                return core::video_fmt::x1080p2997;
            case bmdModeHD1080p30:
                return core::video_fmt::x1080p3000;
            case bmdModeHD1080p50:
                return core::video_fmt::x1080p5000;
            case bmdModeHD1080p5994:
                return core::video_fmt::x1080p5994;
            case bmdModeHD1080p6000:
                return core::video_fmt::x1080p6000;
            case bmdMode2k2398:
                return core::video_fmt::x1556p2398;
            case bmdMode2k24:
                return core::video_fmt::x1556p2400;
            case bmdMode2k25:
                return core::video_fmt::x1556p2500;
            case bmdMode4K2160p2398:
                return core::video_fmt::x2160p2398;
            case bmdMode4K2160p24:
                return core::video_fmt::x2160p2400;
            case bmdMode4K2160p25:
                return core::video_fmt::x2160p2500;
            case bmdMode4K2160p2997:
                return core::video_fmt::x2160p2997;
            case bmdMode4K2160p30:
                return core::video_fmt::x2160p3000;
            case bmdMode4K2160p50:
                return core::video_fmt::x2160p5000;
            case bmdMode4K2160p5994:
                return core::video_fmt::x2160p5994;
            case bmdMode4K2160p60:
                return core::video_fmt::x2160p6000;
            default:
                return core::video_fmt::invalid;
        }
    }
} //namespace seeder::decklink