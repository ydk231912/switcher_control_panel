
#pragma once

namespace seeder::rtp
{
    // video timestamp wrap around, 2^32
    constexpr auto RTP_WRAP_AROUND = 0x100000000;

    // TDRAIN = (TFRAME / NPACKETS) * ( 1 / β ), β is the scaling factor = 1.1,
    // the TDRAIN means the time for one packet to send
    constexpr auto PACKET_DRAIN_RATE = 1.1;

    // video media clock rate, 90kHz
    constexpr auto RTP_VIDEO_RATE = 90000;

    // audio media clock rate, 48kHz or 96kHz
    constexpr auto RTP_AUDIO_RATE_48 = 48000;
    constexpr auto RTP_AUDIO_RATE_96 = 96000;

    // leap second, since 1970-01-01 to 2022-01-01 
    constexpr auto LEAP_SECONDS = 37;
}