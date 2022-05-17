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

#include <string>
#include <memory>

#include "interop/DeckLinkAPI.h"
#include "util/video_format.h"

namespace seeder::decklink
{
    /**
     * @brief Get the decklink video format object
     * 
     * @param fmt 
     * @return BMDDisplayMode 
     */
    static BMDDisplayMode get_decklink_video_format(util::video_format fmt)
    {
        switch (fmt) {
            case util::video_format::pal:
                return bmdModePAL;
            case util::video_format::ntsc:
                return bmdModeNTSC;
            case util::video_format::x576p2500:
                return (BMDDisplayMode)ULONG_MAX;
            case util::video_format::x720p2398:
                return (BMDDisplayMode)ULONG_MAX;
            case util::video_format::x720p2400:
                return (BMDDisplayMode)ULONG_MAX;
            case util::video_format::x720p2500:
                return (BMDDisplayMode)ULONG_MAX;
            case util::video_format::x720p5000:
                return bmdModeHD720p50;
            case util::video_format::x720p2997:
                return (BMDDisplayMode)ULONG_MAX;
            case util::video_format::x720p5994:
                return bmdModeHD720p5994;
            case util::video_format::x720p3000:
                return (BMDDisplayMode)ULONG_MAX;
            case util::video_format::x720p6000:
                return bmdModeHD720p60;
            case util::video_format::x1080p2398:
                return bmdModeHD1080p2398;
            case util::video_format::x1080p2400:
                return bmdModeHD1080p24;
            case util::video_format::x1080i5000:
                return bmdModeHD1080i50;
            case util::video_format::x1080i5994:
                return bmdModeHD1080i5994;
            case util::video_format::x1080i6000:
                return bmdModeHD1080i6000;
            case util::video_format::x1080p2500:
                return bmdModeHD1080p25;
            case util::video_format::x1080p2997:
                return bmdModeHD1080p2997;
            case util::video_format::x1080p3000:
                return bmdModeHD1080p30;
            case util::video_format::x1080p5000:
                return bmdModeHD1080p50;
            case util::video_format::x1080p5994:
                return bmdModeHD1080p5994;
            case util::video_format::x1080p6000:
                return bmdModeHD1080p6000;
            case util::video_format::x1556p2398:
                return bmdMode2k2398;
            case util::video_format::x1556p2400:
                return bmdMode2k24;
            case util::video_format::x1556p2500:
                return bmdMode2k25;
            case util::video_format::x2160p2398:
                return bmdMode4K2160p2398;
            case util::video_format::x2160p2400:
                return bmdMode4K2160p24;
            case util::video_format::x2160p2500:
                return bmdMode4K2160p25;
            case util::video_format::x2160p2997:
                return bmdMode4K2160p2997;
            case util::video_format::x2160p3000:
                return bmdMode4K2160p30;
            case util::video_format::x2160p5000:
                return bmdMode4K2160p50;
            case util::video_format::x2160p5994:
                return bmdMode4K2160p5994;
            case util::video_format::x2160p6000:
                return bmdMode4K2160p60;
            default:
                return (BMDDisplayMode)ULONG_MAX;
        }
    }

    /**
     * @brief Get the seeder video format object
     * 
     * @param fmt 
     * @return util::video_format 
     */
    static util::video_format get_seeder_video_format(BMDDisplayMode fmt)
    {
        switch (fmt) {
            case bmdModePAL:
                return util::video_format::pal;
            case bmdModeNTSC:
                return util::video_format::ntsc;
            case bmdModeHD720p50:
                return util::video_format::x720p5000;
            case bmdModeHD720p5994:
                return util::video_format::x720p5994;
            case bmdModeHD720p60:
                return util::video_format::x720p6000;
            case bmdModeHD1080p2398:
                return util::video_format::x1080p2398;
            case bmdModeHD1080p24:
                return util::video_format::x1080p2400;
            case bmdModeHD1080i50:
                return util::video_format::x1080i5000;
            case bmdModeHD1080i5994:
                return util::video_format::x1080i5994;
            case bmdModeHD1080i6000:
                return util::video_format::x1080i6000;
            case bmdModeHD1080p25:
                return util::video_format::x1080p2500;
            case bmdModeHD1080p2997:
                return util::video_format::x1080p2997;
            case bmdModeHD1080p30:
                return util::video_format::x1080p3000;
            case bmdModeHD1080p50:
                return util::video_format::x1080p5000;
            case bmdModeHD1080p5994:
                return util::video_format::x1080p5994;
            case bmdModeHD1080p6000:
                return util::video_format::x1080p6000;
            case bmdMode2k2398:
                return util::video_format::x1556p2398;
            case bmdMode2k24:
                return util::video_format::x1556p2400;
            case bmdMode2k25:
                return util::video_format::x1556p2500;
            case bmdMode4K2160p2398:
                return util::video_format::x2160p2398;
            case bmdMode4K2160p24:
                return util::video_format::x2160p2400;
            case bmdMode4K2160p25:
                return util::video_format::x2160p2500;
            case bmdMode4K2160p2997:
                return util::video_format::x2160p2997;
            case bmdMode4K2160p30:
                return util::video_format::x2160p3000;
            case bmdMode4K2160p50:
                return util::video_format::x2160p5000;
            case bmdMode4K2160p5994:
                return util::video_format::x2160p5994;
            case bmdMode4K2160p60:
                return util::video_format::x2160p6000;
            default:
                return util::video_format::invalid;
        }
    }
} //namespace seeder::decklink