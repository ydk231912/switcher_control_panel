/**
 * @file packets_buffer.cpp
 * @author 
 * @brief rtp packets ring buffer
 * @version 1
 * @date 2022-08-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <malloc.h>

#include "rtp/packets_buffer.h"
#include "rtp/header.h"
#include "st2110/d10/network.h"
#include "st2110/d20/raw_line_header.h"

namespace seeder::rtp
{
    packets_buffer::packets_buffer()
    :capacity_(15376)  //22.3MB, one 4k frame
    {
        init();
    }

    packets_buffer::packets_buffer(uint32_t capacity)
    :capacity_(capacity)
    {
        init();
    }

    packets_buffer::~packets_buffer()
    {
        if(data_)
            free(data_);
    }

    void packets_buffer::init()
    
    {
        in_ = 0;
        out_ = 0;
        size_ = 0;
        // allocate memory
        data_ = (uint8_t*)malloc(capacity_ * d10::STANDARD_UDP_SIZE_LIMIT);

        packet_info_ = std::vector<packet_info>(capacity_);
        // for(int i = 0; i < capacity_; i++ )
        // {
        //     packets_size_[i] = d10::STANDARD_UDP_SIZE_LIMIT;
        //     packets_first_flag_[i] = false;
        // }
    }

    /**
     * @brief write the data to buffer
     * 
     * @return uint32_t the write packet position, -1: failed
     */
    int packets_buffer::write()
    {
        int index = -1;
        if(size_ < (capacity_ - 2))
        {
            index = in_;
            packet_info_[index].size = d10::STANDARD_UDP_SIZE_LIMIT;
            packet_info_[index].first_of_frame = false;
            in_++;
            if(in_ == capacity_) in_ = 0;
            //size_++;
        }
        
        return index;
    }

    void packets_buffer::write_complete()
    {   
        size_++;
    }

    /**
     * @brief read the data from buffer
     * 
     * @return uint32_t the read packet position, -1: failed
     */
    int packets_buffer::read()
    {
        int index = -1;
        if(size_ > 0)
        {
            index = out_;
            out_++;
            if(out_ == capacity_) out_ = 0;
            //size_--;
        }
        return index;
    }

    void packets_buffer::read_complete()
    {
        size_--;
    }

    /**
     * @brief Get the packet pointer
     * 
     * @param index the packet position of the buffer
     * @return uint8_t* the packet begin pointer
     */
    uint8_t* packets_buffer::get_packet_ptr(int index)
    {
        uint8_t* ptr = nullptr;
        if(index >= 0 && index < capacity_)
        {
            ptr = data_ + index * d10::STANDARD_UDP_SIZE_LIMIT;
        }

        return ptr;
    }

    /**
     * @brief Get the packet payload ptr
     * 
     * @return uint8_t* 
     */
    uint8_t* packets_buffer::get_payload_ptr(int index)
    {
        return data_ + index * d10::STANDARD_UDP_SIZE_LIMIT + sizeof(raw_header);
    }

    /**
     * @brief Get the packet payload size
     * 
     * @return int 
     */
    int packets_buffer::get_payload_size()
    {
        return d10::STANDARD_UDP_SIZE_LIMIT - sizeof(raw_header);
    }

    /**
     * @brief is the current packet is the first packet of the frame
     * 
     * @param index 
     * @return true 
     * @return false 
     */
    bool packets_buffer::is_first_of_frame(int index)
    {
        return packet_info_[index].first_of_frame;
    }

    /**
     * @brief Set the packet is the first of frame
     * 
     * @param packet_ptr 
     * @param first_packet 
     */
    void packets_buffer::set_first_of_frame(int index)
    {
        packet_info_[index].first_of_frame = true;
    }

    /**
     * @brief Set the packet rtp heard
     * 
     * @param packet_ptr 
     * @param payload_type 
     */
    void packets_buffer::set_header(uint8_t* packet_ptr, uint8_t payload_type)
    {
        if (!packet_ptr) return;

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
        uint8_t marker = 0;

        packet_ptr[0] = (csrc_count & 0x0f) | ((extension << 4 ) & 0x10) | ((padding << 5) & 0x20) | ((version << 6) & 0xc0);
        packet_ptr[1] = (payload_type & 0x7f) | ((marker << 7) & 0x80);
        packet_ptr[2] = 0; //sequence number 16bit
        packet_ptr[3] = 0;
        packet_ptr[4] = 0; //timestamp 32bit
        packet_ptr[5] = 0;
        packet_ptr[6] = 0;
        packet_ptr[7] = 0;
        packet_ptr[8] = 0; //SSRC 32bit
        packet_ptr[9] = 0;
        packet_ptr[10] = 0;
        packet_ptr[11] = 0;
    }

    /**
     * @brief Set the packet rtp marker, indicate the last packet of the frame
     * 
     * @param packet_ptr 
     * @param payload_type 
     */
    void packets_buffer::set_marker(uint8_t* packet_ptr, uint8_t  payload_type, uint8_t marker)
    {
        packet_ptr[1] = (payload_type & 0x7f) | ((marker << 7) & 0x80);
    }

    /**
     * @brief Set the packet rtp sequence number
     * 
     * @param packet_ptr 
     * @param sequence_number 
     */
    void packets_buffer::set_sequence_number(uint8_t* packet_ptr, uint16_t sequence_number)
    {
         packet_ptr[2] = (sequence_number >> 8) & 0xff; // big-endian
         packet_ptr[3] = sequence_number & 0xff;
    }

    /**
     * @brief Set the packet rtp timestamp
     * 
     * @param packet_ptr 
     * @param timestamp 
     */
    void packets_buffer::set_timestamp(uint8_t* packet_ptr, uint32_t timestamp)
    {
        packet_ptr[4]       = (timestamp >> 24) & 0xff; // big-endian
        packet_ptr[5]       = (timestamp >> 16) & 0xff; // big-endian
        packet_ptr[6]       = (timestamp >> 8) & 0xff; // big-endian
        packet_ptr[7]       = timestamp & 0xff;
    }

    uint32_t packets_buffer::get_packet_size()
    {
        return d10::STANDARD_UDP_SIZE_LIMIT;
    }
        
    void packets_buffer::set_packet_real_size(int index, uint32_t size)
    {
        packet_info_[index].size = size;
    }

    uint32_t packets_buffer::get_packet_real_size(int index)
    {
        return packet_info_[index].size;
    }

    void packets_buffer::set_frame_time(int index, uint64_t time)
    {
        packet_info_[index].frame_time = time;
    }

    uint64_t packets_buffer::get_frame_time(int index)
    {
        return packet_info_[index].frame_time;
    }

    void packets_buffer::set_frame_id(int index, uint32_t id)
    {
        packet_info_[index].frame_id = id;
    }

    uint32_t packets_buffer::get_frame_id(int index)
    {
        return packet_info_[index].frame_id;
    }

    uint64_t packets_buffer::get_buffer_size()
    {
        return size_;
    }

}