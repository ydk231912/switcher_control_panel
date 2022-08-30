/**
 * st2110-10 relevant global const definition
 */
#pragma once

#include <cstdint>

namespace seeder { namespace d10 {
    /*
    The Standard UDP size limit shall be 1460 octets. The UDP size is reflected in the UDP header, and includes 
    the length of the UDP header (8 octets) and also the RTP headers and data. Senders shall not generate IP 
    datagrams containing UDP packet sizes larger than this limit unless operating conformant to the optional 
    extended UDP size limit specified in section 6.4. Regardless of the presence or size of any RTP header 
    extensions, senders shall adhere to the UDP size constraints.
    */
    constexpr uint16_t STANDARD_UDP_SIZE_LIMIT = 1452; //1370; // 1460 - 8;
    constexpr uint16_t EXTENDED_UDP_SIZE_LIMIT = 8960 - 8;
    constexpr uint16_t MAX_PACKET_SIZE = 1900;

}} // namespace seeder::d10
