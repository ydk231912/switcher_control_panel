/**
 * rtp packet
 *
 */
#include "packet.h"
#include "header.h"
#include "st2110/d20/raw_line_header.h"
#include "st2110/d10/network.h"

#include <malloc.h>

namespace seeder::rtp {

    packet::packet()
    {
        data_ = (uint8_t*)malloc(d10::STANDARD_UDP_SIZE_LIMIT);
        length_      = d10::STANDARD_UDP_SIZE_LIMIT;
        
        uint8_t* ptr = data_;
        if (!ptr)
            return;

        // from https://tools.ietf.org/html/rfc3550
        //    0                   1                   2                   3
        //    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
        //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //   |                           timestamp                           |
        //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //   |           synchronization source (SSRC) identifier            |
        //   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
        //   |            contributing source (CSRC) identifiers             |
        //   |                             ....                              |
        //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        uint8_t csrc_count = 0;
        uint8_t extension = 0;
        uint8_t padding = 0;
        uint8_t version = 2;
        uint8_t payload_type = 96; // 96 97 102  103
        uint8_t marker       = 0;

        //*ptr++ = ((csrc_count << 4) & 0xf0) | ((extension << 3) & 0x08) | ((padding << 2) & 0x04) | (version & 0x03);
        //*ptr++ = ((payload_type << 1) & 0xfe) | (marker & 0x01);
        *ptr++ = (csrc_count & 0x0f) | ((extension << 4 ) & 0x10) | ((padding << 5) & 0x20) | ((version << 6) & 0xc0);
        *ptr++ = (payload_type & 0x7f) | ((marker << 7) & 0x80);
        *ptr++ = 0; //sequence number 16bit
        *ptr++ = 0;
        *ptr++ = 0; //timestamp 32bit
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0; //SSRC 32bit
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
    }

    packet::packet(int length)
    {
        data_ = (uint8_t*)malloc(length);
        length_ = length;
    }

    packet::~packet()
    {
        if(data_)
            free(data_);
    }

    int  packet::get_size()
    {
        return length_;
    }

    void packet::reset_size(int length)
    {
        length_ = length;
    }

    /**
     * get the payload starting pointer
     */
    uint8_t* packet::get_payload_ptr()
    {
        return data_ + sizeof(raw_header);
    }

    /**
     * get the rtp packet data starting pointer
     */
    uint8_t* packet::get_data_ptr()
    {
        return data_;
    }

    /**
     * get the payload size
     */
    int packet::get_payload_size()
    {
        return length_ - sizeof(raw_header);
    }

    /**
     * get the rtp packet data size
     */
    int packet::get_data_size()
    {
        return length_;
    }

    void packet::set_marker(uint8_t marker)
    {
        uint8_t* ptr   = data_ + 1;
        uint8_t  payload_type = 96;
        *ptr = (payload_type & 0x7f) | ((marker << 7) & 0x80);
        set_last_of_frame(true);
    }

    void packet::set_sequence_number(uint16_t sequence_number)
    {
        uint8_t* ptr            = data_ + 2;
         *ptr++ = (sequence_number >> 8) & 0xff; // big-endian
         *ptr = sequence_number & 0xff;
        //*ptr++ = sequence_number & 0xff;
        //*ptr = (sequence_number >> 8) & 0xff;
    }

    void packet::set_timestamp(uint32_t timestamp)
    {
        uint8_t* ptr = data_ + 4;
        *ptr++       = (timestamp >> 24) & 0xff; // big-endian
        *ptr++       = (timestamp >> 16) & 0xff; // big-endian
        *ptr++       = (timestamp >> 8) & 0xff; // big-endian
        *ptr         = timestamp & 0xff;
        //*ptr++       = timestamp & 0xff;
        //*ptr++       = (timestamp >> 8) & 0xff;
        //*ptr++       = (timestamp >> 16) & 0xff;
        //*ptr         = (timestamp >> 24) & 0xff;
    }

    void packet::set_first_of_frame(bool first_packet)
    {
        first_of_frame_ = first_packet;
    }

    bool packet::get_first_of_frame()
    {
        return first_of_frame_;
    }

    void packet::set_last_of_frame(bool last_packet)
    {
        last_of_frame_ = last_packet;
    }
}
