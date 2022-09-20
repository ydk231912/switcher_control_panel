// /**
//  * @file rtp_st2110_output.h
//  * @author 
//  * @brief recieve frame, encodec to st2110 packet, send packet by udp
//  * @version 1.0
//  * @date 2022-07-27
//  * 
//  * @copyright Copyright (c) 2022
//  * 
//  */

// #pragma once

// #include <memory>
// #include <mutex>
// #include <deque>
// #include <condition_variable>
// #include <thread>
// #include <boost/lockfree/spsc_queue.hpp>

// #include "core/stream/output.h"
// #include "rtp/packet.h"
// #include "net/udp_sender.h"
// #include "rtp/rtp_context.h"
// #include "core/executor/executor.h"
// #include "rtp/packets_buffer.h"

// using namespace boost::lockfree;
// namespace seeder::rtp
// {
//     class rtp_st2110_output : public core::output
//     {
//       public:
//         rtp_st2110_output(rtp_context& context);
//         ~rtp_st2110_output();

//         /**
//          * @brief start output stream handle
//          * 
//          */
//         void start();
            
//         /**
//          * @brief stop output stream handle
//          * 
//          */
//         void stop();

//         /**
//          * @brief encode the frame data into st2110 rtp packet
//          * one frame may be divided into multiple fragment in multiple thread 
//          */
//         void encode();
        
//         /**
//          * @brief encode the video frame data into st2110 rtp packet
//          * 
//          * @param queue_id  buffer queue/thread id
//          * @param frm vidoe data
//          * @param line frame start line number
//          * @param height frame end line number
//          */
//         void video_encode(uint16_t queue_id, std::shared_ptr<core::frame> frm, uint16_t line, uint16_t height);

//         /**
//          * @brief encode the audio data into st2110 rtp packet
//          * 
//          * @param queue_id buffer queue/thread id
//          * @param frm audio data
//          */
//         void audio_encode(uint16_t queue_id, std::shared_ptr<core::frame> frm);

//         /**
//          * @brief calculate rtp packets number per frame
//          * 
//          * @return int 
//          */
//         int calc_packets_per_frame(int line, int height);

//         /**
//          * @brief send the rtp packet by udp
//          * 
//          */
//         void send_packet(uint16_t queue_id);
//         void send(uint16_t queue_id);

//         /**
//          * @brief push a frame into this output stream
//          * 
//          */
//         void set_frame(std::shared_ptr<core::frame> frm);

//         /**
//          * @brief Get a frame from this output stream
//          * 
//          */
//         std::shared_ptr<core::frame> get_frame();

//         /**
//          * @brief Get the st2110 packet 
//          * 
//          * @return std::shared_ptr<packet> 
//          */
//         std::shared_ptr<packet> get_packet();

//         /**
//          * @brief Set the packet object
//          * 
//          */
//         void set_packet(std::shared_ptr<packet> packet);
      
//       private:
//         rtp_context context_;
//         bool abort = false;
//         atomic<uint16_t> sequence_number_; //video rtp sequence number
//         std::vector<uint16_t> sequences_; // video rtp sequence number, per thread
//         std::vector<int> packets_number_; // packets number per frame per thread
//         atomic<uint32_t> timestamp_; // video rtp timestamp
//         atomic<bool> first_of_frame_;
//         uint64_t time_now_; // frame timestamp
//         uint16_t audio_sequence_number_; //audio rtp sequence number
//         uint32_t audio_timestamp_; // audio rtp timestamp

//         //buffer and thread
//         std::mutex frame_mutex_, packet_mutex_;
//         std::deque<std::shared_ptr<core::frame>> frame_buffer_;
//         std::deque<std::shared_ptr<rtp::packet>> packet_buffer_;
        
//         const std::size_t frame_capacity_ = 3;
//         std::size_t packet_capacity_ = 11544; // 3frame 

//         std::condition_variable frame_cv_;
//         std::condition_variable packet_cv_;
//         std::unique_ptr<std::thread> st2110_thread_;
//         //std::unique_ptr<core::executor> executor_;
        
//         // udp sender
//         std::vector<std::shared_ptr<rtp::packets_buffer>> packets_buffers_;
//         //int16_t qid_ = -1; // packets_buffers_ queue index
//         std::vector<std::shared_ptr<net::udp_sender>> udp_senders_;
//         std::vector<std::shared_ptr<std::thread>> udp_threads_;
//         std::vector<int64_t> drain_offsets_; // udp sender queues, send packet time offsets
//         uint64_t frame_time_;
//         uint32_t frame_id_ = 1;
//         uint64_t packet_drain_interval_;
//         int packets_per_frame_ = 0;

//         //int64_t packet_drained_number_ = 0;
//         //int64_t frame_number_ = 0;

//     };
// }
