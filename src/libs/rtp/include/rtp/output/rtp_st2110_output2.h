/**
 * @file rtp_st2110_output.h
 * @author 
 * @brief recieve frame, encodec to st2110 packet, send packet by udp
 * @version 1.0
 * @date 2022-07-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <memory>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <thread>
#include <boost/lockfree/spsc_queue.hpp>

#include "core/stream/output.h"
#include "rtp/packet.h"
#include "net/udp_sender.h"
#include "rtp/rtp_context.h"
#include "core/executor/executor.h"

using namespace boost::lockfree;
namespace seeder::rtp
{
    class rtp_st2110_output2 : public core::output
    {
      public:
        rtp_st2110_output2(const std::string &output_id, rtp_context& context);
        ~rtp_st2110_output2();

        /**
         * @brief start output stream handle
         * 
         */
        void start();
            
        /**
         * @brief stop output stream handle
         * 
         */
        void stop();

        /**
         * @brief encode the frame data into st2110 rtp packet
         * 
         */
        void encode();
        void do_encode();

        /**
         * @brief calculate rtp packets number per frame
         * 
         * @return int 
         */
        int calc_packets_per_frame();

        /**
         * @brief send the rtp packet by udp
         * 
         */
        void send_packet();
        void send_packet2();

        /**
         * @brief push a frame into this output stream
         * 
         */
        void set_frame(std::shared_ptr<core::frame> frm);

        /**
         * @brief Get a frame from this output stream
         * 
         */
        std::shared_ptr<core::frame> get_frame();

        /**
         * @brief Get the st2110 packet 
         * 
         * @return std::shared_ptr<packet> 
         */
        std::shared_ptr<packet> get_packet();

        /**
         * @brief Set the packet object
         * 
         */
        void set_packet(std::shared_ptr<packet> packet);
      
      private:
        rtp_context context_;
        bool abort = false;
        uint16_t sequence_number_;
        std::mutex frame_mutex_, packet_mutex_;
        std::deque<std::shared_ptr<core::frame>> frame_buffer_;
        std::deque<std::shared_ptr<rtp::packet>> packet_buffer_;
        std::deque<std::shared_ptr<rtp::packet>> packet_buffer2_;
        const std::size_t frame_capacity_ = 3;
        std::size_t packet_capacity_ = 11544; // 3frame 

        std::condition_variable frame_cv_;
        std::condition_variable packet_cv_;
        std::unique_ptr<std::thread> st2110_thread_;
        std::unique_ptr<std::thread> udp_thread_;
        std::unique_ptr<std::thread> udp_thread2_;
        uint8_t qid_ = 0;

        uint16_t height_;
        
        std::unique_ptr<net::udp_sender> udp_sender_ = nullptr;
        std::unique_ptr<net::udp_sender> udp_sender2_ = nullptr;
        std::vector<std::shared_ptr<net::udp_sender>> udp_senders_;
        int sender_ptr_ = 0;
        std::unique_ptr<core::executor> executor_;

        int64_t start_time_;
        int64_t packet_drain_interval_;
        int64_t packet_drained_number_ = 0;
        int64_t frame_number_ = 0;

        int packets_per_frame_ = 0;


        std::unique_ptr<std::thread> temp_thread_;

        spsc_queue<std::shared_ptr<rtp::packet>, capacity<4800000>> packet_bf_;
        spsc_queue<std::shared_ptr<rtp::packet>, capacity<4800000>> packet_bf2_;
        int timestampe_;

    };
}
