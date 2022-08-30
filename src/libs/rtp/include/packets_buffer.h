/**
 * @file packets_buffer.h
 * @author 
 * @brief rtp packets ring buffer
 * one write thread, one read thread, lock free
 * @version 1
 * @date 2022-08-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <atomic>
#include <vector>

namespace seeder::rtp
{
    struct packet_info
    {
      uint32_t size; // packet real size
      bool first_of_frame; // is first packet of the frame
      uint64_t frame_time; 
    };

    //                   packets buffer(one packet 1452 bytes)
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    0             1451             2903           4355                
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    | packet1       | packet2       | packet3       |                 
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //       out = 0                           in = 2                       
    class packets_buffer
    {
      public:
        packets_buffer();
        packets_buffer(uint32_t capacity);
        ~packets_buffer();

        /**
         * @brief write the data to buffer
         * 
         * @return uint32_t the write packet position, -1: failed
         */
        int write();

        /**
         * @brief read the data to buffer
         * 
         * @return uint32_t the read packet position, -1: failed
         */
        int read();

        /**
         * @brief Get the packet pointer 
         * 
         * @param index the packet position of the buffer
         * @return uint8_t* the packet begin pointer
         */
        uint8_t* get_packet_ptr(int index);

        /**
         * @brief Get the packet payload ptr 
         * 
         * @return uint8_t* 
         */
        uint8_t* get_payload_ptr(int index);

        /**
         * @brief Get the packet payload size
         * 
         * @return int 
         */
        int get_payload_size();

        /**
         * @brief is the current packet is the first packet of the frame
         * 
         * @param index 
         * @return true 
         * @return false 
         */
        bool is_first_of_frame(int index);

        /**
         * @brief Set the packet is the first of frame
         * 
         * @param packet_ptr 
         * @param first_packet 
         */
        void set_first_of_frame(int index);

        /**
         * @brief Set the packet rtp header
         * 
         * @param packet_ptr 
         * @param payload_type 
         */
        void set_header(uint8_t* packet_ptr, uint8_t payload_type);

        /**
         * @brief Set the packet rtp marker, indicate the last packet of the frame
         * 
         * @param packet_ptr 
         * @param payload_type 
         */
        void set_marker(uint8_t* packet_ptr, uint8_t  payload_type, uint8_t marker);

        /**
         * @brief Set the packet rtp sequence number
         * 
         * @param packet_ptr 
         * @param sequence_number 
         */
        void set_sequence_number(uint8_t* packet_ptr, uint16_t sequence_number);

        /**
         * @brief Set the packet rtp timestamp
         * 
         * @param packet_ptr 
         * @param timestamp 
         */
        void set_timestamp(uint8_t* packet_ptr, uint32_t timestamp);

        uint32_t get_packet_size();

        void set_packet_real_size(int index, uint32_t size);
        uint32_t get_packet_real_size(int index);

        void set_frame_time(int index, uint64_t time);
        uint64_t get_frame_time(int index);

      private:
        void init();

      private:
        uint8_t* data_;
        uint32_t capacity_; // packets number
        std::atomic<uint32_t> size_;  // current how many packets in the buffer
        uint32_t in_;  // write packet position
        uint32_t out_; // read packet position

        std::vector<packet_info> packet_info_; // packet real size
    };
}