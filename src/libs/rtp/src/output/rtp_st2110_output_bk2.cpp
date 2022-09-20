// /**
//  * @file rtp_st2110_output.cpp
//  * @author 
//  * @brief recieve frame, encodec to st2110 packet, send packet by udp
//  * @version 1.0
//  * @date 2022-07-27
//  * 
//  * @copyright Copyright (c) 2022
//  * 
//  */

// #pragma once

// #include <iostream>
// #include <exception>
// #include <chrono>
// #include <boost/date_time/posix_time/posix_time.hpp>
// #include <boost/chrono.hpp>
// #include <boost/thread/thread.hpp>
// #include <unistd.h>

// extern "C"
// {
//     #include <libavformat/avformat.h>
// }

// #include "core/util/logger.h"
// #include "core/util/timer.h"
// #include "core/util/date.h"
// #include "st2110/d20/raw_line_header.h"
// #include "st2110/d10/network.h"
// #include "rtp/header.h"
// #include "rtp/rtp_constant.h"

// #include "output/rtp_st2110_output.h"

// using namespace seeder::core;
// namespace seeder::rtp
// {
//     rtp_st2110_output::rtp_st2110_output(rtp_context& context)
//     :context_(context)
//     //,executor_(std::make_unique<core::executor>())
//     {
//         udp_senders_.push_back(std::make_shared<net::udp_sender>());
//         udp_senders_.push_back(std::make_shared<net::udp_sender>());
//         //udp_senders_[0]->bind_ip("192.168.10.103", 20001);
//         //udp_senders_[1]->bind_ip("192.168.10.103", 20001);
//         packets_buffers_.push_back(std::make_shared<packets_buffer>());
//         packets_buffers_.push_back(std::make_shared<packets_buffer>());

//         uint16_t line1 = 0;
//         uint16_t line2 = context_.format_desc.height / 2;
//         uint16_t height1 = line2;
//         uint16_t height2 = context_.format_desc.height;

//         packets_number_.push_back(calc_packets_per_frame(line1, height1));
//         packets_number_.push_back(calc_packets_per_frame(line2, height2));
//         packets_per_frame_ = packets_number_[0] + packets_number_[1];
//         int64_t frame_duration  = 1000000000 / context_.format_desc.fps; //nanosecond
//         packet_drain_interval_ = static_cast<int64_t>((frame_duration / packets_per_frame_) / PACKET_DRAIN_RATE); //two send queue
//         packet_drain_interval_ = packet_drain_interval_ * 2;
//         drain_offsets_.push_back(0);
//         drain_offsets_.push_back(packet_drain_interval_ / 2);

//         sequence_number_ = 0;
//         timestamp_ = 0;
//         audio_sequence_number_ = 0;
//         audio_timestamp_ = 0;
//         frame_id_ = 1;
//         sequences_.push_back(0);
//         sequences_.push_back(packets_number_[0]);
//     }

//     rtp_st2110_output::~rtp_st2110_output()
//     {
//     }

//     /**
//      * @brief start output stream handle
//      * 
//      */
//     void rtp_st2110_output::start()
//     {
//         abort = false;
//         encode();
//         // send_packet(0);
//         // send_packet(1);
//     }
        
//     /**
//      * @brief stop output stream handle
//      * 
//      */
//     void rtp_st2110_output::stop()
//     {
//         abort = true;
//     }

//     /**
//      * @brief encode the frame data into st2110 rtp packet
//      * one frame may be divided into multiple fragment in multiple thread 
//      */
//     void rtp_st2110_output::encode()
//     {
//         st2110_thread_ = std::make_unique<std::thread>(std::thread([&](){
//             while(!abort) 
//             {
//                 //get frame from buffer
//                 auto frame = get_frame();
//                 if(!frame->video)
//                 {
//                     continue;
//                 }
                
//                 if (frame->video->format != AV_PIX_FMT_YUV422P && frame->video->format != AV_PIX_FMT_UYVY422)
//                 {
//                     //currently,only YUV422P UYVY422 format is supported
//                     logger->error("The {} frame format is unsupported!", frame->video->format);
//                     throw std::runtime_error("The frame format is unsupported!");
//                 }

//                 uint16_t line1 = 0;
//                 uint16_t line2 = frame->video->height / 2;
//                 uint16_t height1 = line2;
//                 uint16_t height2 = frame->video->height;
//                 first_of_frame_ = true;
                
//                 // video frame encode in two thread
//                 // executor_->execute(std::move([this, frame, line1, height2](){
//                 //     //timer timer;
//                 //     video_encode(0, frame, line1, height2);
//                 //     //std::cout << "thread1 video frame encode ns: " << timer.elapsed() << "  buffer size: " << packets_buffers_[0]->get_buffer_size() << std::endl;
//                 // }));

//                 time_now_ = date::now() + context_.leap_seconds * 1000000; //microsecond. 37s: leap second, since 1970-01-01 to 2022-01-01
//                 auto video_time = static_cast<uint64_t>(round(time_now_ * RTP_VIDEO_RATE / 1000000));
//                 timestamp_ = static_cast<uint32_t>(video_time % RTP_WRAP_AROUND);
//                 frame->timestamp = timestamp_;
//                 frame->frame_id = frame_id_;
//                 frame_id_ ++;

//                 auto t1 = std::thread([&](){
//                     video_encode(0, frame, line1, height1);
//                 });
//                 t1.join();
//                 send(0);

//                 auto t2 = std::thread([&](){
//                     video_encode(1, frame, line2, height2);
//                 });
//                 t2.join();
//                 send(1);

//                 // audio encode
//                 //audio_encode(0, frame);

//             }
//         }));
//     }

//     /**
//      * @brief encode the audio data into st2110 rtp packet
//      * audio encode must after of video encode, 
//      * because some data (etc timestamp) from video encode
//      * @param queue_id buffer queue/thread id
//      * @param frm audio data, 8 channel, 16 bit, 48khz
//      */
//     void rtp_st2110_output::audio_encode(uint16_t queue_id, std::shared_ptr<core::frame> frm)
//     {
//         auto audio = frm->audio;

//         if(!audio || !audio->data[0])
//             return;

//         // one packet contains 1ms audio data, must be less than 1440 bytes
//         // 16bit * 8channels * 48000sample rate
//         auto packet_size = context_.format_desc.audio_samples / 8 * context_.format_desc.audio_channels * context_.format_desc.audio_sample_rate / 1000; 
//         auto left = packet_size * context_.format_desc.fps; // one frame audio data size
//         uint8_t* audio_ptr = audio->data[0];

//         uint32_t packet_number = 0;

//         while(left > 0)
//         {
//             int packet_idx = -1; // the packet index of the packets buffer
//             packet_idx = packets_buffers_[queue_id]->write();
//             while(packet_idx == -1)
//             {   // loop until get packet buffer
//                 packet_idx = packets_buffers_[queue_id]->write();
//                 logger->warn("rtp st2110 output: get packet buffer failed");
//             }
//             uint8_t* packet_ptr = packets_buffers_[queue_id]->get_packet_ptr(packet_idx);
//             packets_buffers_[queue_id]->set_header(packet_ptr, context_.rtp_audio_type);
//             uint8_t* ptr = packets_buffers_[queue_id]->get_payload_ptr(packet_idx);

//             // one packet contains 1ms audio data
//             int size = left < packet_size? left : packet_size;
//             memcpy(ptr, audio_ptr, size);
//             ptr += size;
            
//             audio_sequence_number_++;
//             packets_buffers_[queue_id]->set_sequence_number(packet_ptr, audio_sequence_number_);

//             if (left <= packet_size) { //complete
//                 packets_buffers_[queue_id]->set_marker(packet_ptr, context_.rtp_audio_type, 1);
//             }

//             int truncate = packets_buffers_[queue_id]->get_payload_size() - size;
//             packets_buffers_[queue_id]->set_packet_real_size(packet_idx, packets_buffers_[queue_id]->get_packet_size() - truncate);

//             packets_buffers_[queue_id]->set_frame_time(packet_idx, frame_time_); // from video encode
//             // audio timestamp, each packet timestamp increase 1ms
//             // time_now_ from video encode
//             auto audio_time = static_cast<uint64_t>(round((time_now_ + packet_number * 1000) * context_.format_desc.audio_sample_rate / 1000000));
//             audio_timestamp_ = static_cast<uint32_t>(audio_time % RTP_WRAP_AROUND);
//             packets_buffers_[queue_id]->set_timestamp(packet_ptr, audio_timestamp_); // from video encode
//             packet_number++;
            
//             left -= packet_size;
//             if(left > 0) audio_ptr += packet_size;
//         }
//     }

//     /**
//      * @brief encode the video frame data into st2110 rtp packet
//      * 
//      * @param queue_id  buffer queue/thread id
//      * @param frm vidoe data
//      * @param line frame start line number
//      * @param height frame end line number
//      */
//     void rtp_st2110_output::video_encode(uint16_t queue_id, std::shared_ptr<core::frame> frm, uint16_t line, uint16_t height)
//     {
//         timer timer; // for debug

//         auto frame = frm;
//         auto qid = queue_id;

//         int packet_count = 0;
//         uint16_t pgroup = 5; // uyvy422 10bit, 5bytes
//         uint16_t xinc   = 2;
//         uint16_t yinc   = 1;
//         uint16_t offset = 0;
//         uint16_t width = frame->video->width;

//         while(line < height) {
//             uint32_t left, length, pixels;
//             uint8_t  continue_flag;
//             bool next_line = false;
//             uint8_t* headers;

//             int packet_idx = -1; // the packet index of the packets buffer
            
//             packet_idx = packets_buffers_[qid]->write();
//             while(packet_idx == -1)
//             {   // loop until get packet buffer
//                 packet_idx = packets_buffers_[qid]->write();
//                 //logger->warn("rtp st2110 output: get packet buffer failed {}",packets_buffers_[qid]->get_buffer_size());
//             }
//             uint8_t* packet_ptr = packets_buffers_[qid]->get_packet_ptr(packet_idx);

//             if(first_of_frame_ && line == 0)
//             {
//                 first_of_frame_ = false;
//                 packets_buffers_[qid]->set_first_of_frame(packet_idx);
//             }
//             uint8_t* ptr = packets_buffers_[qid]->get_payload_ptr(packet_idx);
//             left = packets_buffers_[qid]->get_payload_size();

//             //RTP Payload Header
//             // from https://tools.ietf.org/html/rfc4175
//             // 0                               16                             32
//             // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//             // |   Extended Sequence Number    |            Length             |
//             // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//             // |F|          Line No            |C|           Offset            |
//             // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//             // |            Length             |F|          Line No            |
//             // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//             // |C|           Offset            |                               .
//             // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               .
//             // .                                                               .
//             // .                 Two (partial) lines of video data             .
//             // .                                                               .
//             // +---------------------------------------------------------------+
//             packets_buffers_[qid]->set_header(packet_ptr, context_.rtp_video_type);
//             ptr[0] = 0;  // extender sequence number 16bit. same as: *ptr = 0; ptr = ptr + 1;
//             ptr[1] = 0;
//             ptr += 2;
//             left -= 2;
//             headers = ptr;
            
//             //// 1 write payload header
//             // while we can fit at least one header and pgroup
//             while (left > (6 + pgroup)) {
//                 left -= 6; // need a 6 bytes payload header(length,line no,offset)

//                 // get how many bytes we need for the remaining pixels
//                 pixels = width - offset;
//                 length = (pixels * pgroup) / xinc; // bytes,
//                 if (left >= length) {
//                     next_line = true;
//                 }
//                 else {
//                     pixels    = (left / pgroup) * xinc;
//                     length    = (pixels * pgroup) / xinc; //left;//;
//                     next_line = false;
//                 }
//                 left -= length;

//                 // write length and line number in payload header
//                 ptr[0]   = (length >> 8) & 0xFF; // big-endian
//                 ptr[1] = length & 0xFF;
//                 ptr[2] = ((line >> 8) & 0x7F) | ((0 << 7) & 0x80); //line no 7bit and "F" flag 1bit
//                 ptr[3] = line & 0xFF;

//                 if (next_line)
//                     line += yinc;

//                 // calculate continuation marker
//                 continue_flag = (left > (6 + pgroup) && line < height) ? 0x80 : 0x00;
//                 // write offset and continuation marker in header
//                 ptr[4] = ((offset >> 8) & 0x7F) | continue_flag;
//                 ptr[5] = offset & 0xFF;
//                 ptr += 6;

//                 if (next_line) { //new line, reset offset
//                     offset = 0; 
//                 } else {
//                     offset += pixels;
//                 }

//                 if (!continue_flag)
//                     break;
//             }

//             //// 2 write data in playload
//             while (true) {
//                 uint32_t offs, ln, i, len, pgs;

//                 // read length and continue flag
//                 len = (headers[0] << 8) | headers[1];  // length
//                 ln    = ((headers[2] & 0x7F) << 8) | headers[3]; // line no
//                 offs   = ((headers[4] & 0x7F) << 8) | headers[5]; // offset
//                 continue_flag = headers[4] & 0x80;
//                 pgs           = len / pgroup; // how many pgroups
//                 headers += 6;

//                 switch (frame->video->format) {
//                     case AV_PIX_FMT_YUV422P:
//                     {
//                         // frame.data[n] 0:Y 1:U 2:V
//                         uint8_t* yp   = frame->video->data[0] + ln * width + offs;            // y point in frame
//                         uint32_t temp     = (ln * width + offs) / xinc;
//                         uint8_t* uvp1 = frame->video->data[1] + temp; // uv point in frame
//                         uint8_t* uvp2 = frame->video->data[2] + temp;
//                         // the graph is the Ycbcr 4:2:2 10bit depth, total 5bytes
//                         // 0                   1                   2                   3
//                         // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
//                         // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//                         // | C’B00 (10 bits)   | Y’00 (10 bits)    | C’R00 (10 bits)   | Y’01 (10 bits)   |
//                         // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//                         for (i = 0; i < pgs; i++) 
//                         {
//                             // 8bit yuv to 10bit ycbcr(CbY0CrY1)
//                             //      Cb         Y0        Cr         Y1
//                             // xxxxxxxx 00++++++ ++00xxxx xxxx00++ ++++++00
//                             //     0        1        2        3        4
//                             ptr[0] = *uvp1;
//                             ptr[1] = *yp >> 2;               //(*yp & 0xFC) >> 2;
//                             ptr[2] = *yp << 6 | *uvp2 >> 4;  // ((*yp & 0x03) << 6) | ((*uvp2 & 0xF0) >> 4);
//                             ++yp;
//                             ptr[3] = *uvp2 << 4 | *yp >> 6;  //((*uvp2 & 0x0F) << 4) | ((*yp & 0xC0) >> 6);
//                             ptr[4] = *yp << 2;               //(*yp & 0x3F) << 2;

//                             ++uvp1;
//                             ++yp;
//                             ++uvp2;
//                             ptr += 5;
//                         }
//                         break;
//                     }
//                     case AV_PIX_FMT_UYVY422:
//                     {
//                         //packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
//                         uint8_t* datap   = frame->video->data[0] + ln * width + offs; 
//                         for (i = 0; i < pgs; i++) 
//                         {
//                             //8bit UYVY(CbY0CrY1) to 10bit ycbcr(CbY0CrY1)
//                             //      Cb         Y0        Cr         Y1
//                             // xxxxxxxx 00++++++ ++00xxxx xxxx00++ ++++++00
//                             //     0        1        2        3        4
//                             // *ptr++ = datap[0];
//                             // *ptr++ = datap[1] >> 2;
//                             // *ptr++ = datap[1] << 6 | datap[2] >> 4;
//                             // *ptr++ = datap[2] << 4 | datap[3] >> 6;
//                             // *ptr++ = datap[3] << 2;
//                             ptr[0] = datap[0];
//                             ptr[1] = datap[1] >> 2;
//                             ptr[2] = datap[1] << 6 | datap[2] >> 4;
//                             ptr[3] = datap[2] << 4 | datap[3] >> 6;
//                             ptr[4] = datap[3] << 2;
//                             datap += 4;
//                             ptr += 5;
//                         }
//                         break;
//                     }
//                 }

//                 if (!continue_flag)
//                     break;
//             }

//             //// 3 write rtp header
//             //sequence_number_++;
//             //packets_buffers_[qid]->set_sequence_number(packet_ptr, sequence_number_);
//             sequences_[qid] += 1;
//             packets_buffers_[qid]->set_sequence_number(packet_ptr, sequences_[qid]);
            
//             if (line >= height && height == frame->video->height) { //complete 
//                 packets_buffers_[qid]->set_marker(packet_ptr, context_.rtp_video_type, 1);
//             } 

//             if (left > 0) {
//                 packets_buffers_[qid]->set_packet_real_size(packet_idx, packets_buffers_[qid]->get_packet_size() - left);
//             }

//             if(packets_buffers_[qid]->is_first_of_frame(packet_idx))
//             {
//                 frame_time_ = timer::now();
//                 // frame_id_ ++;
//                 // time_now_ = date::now() + context_.leap_seconds * 1000000; //microsecond. 37s: leap second, since 1970-01-01 to 2022-01-01
//                 // auto video_time = static_cast<uint64_t>(round(time_now_ * RTP_VIDEO_RATE / 1000000));
//                 // timestamp_ = static_cast<uint32_t>(video_time % RTP_WRAP_AROUND);
//             }
//             packets_buffers_[qid]->set_frame_time(packet_idx, frame_time_);
//             packets_buffers_[qid]->set_frame_id(packet_idx, frame->frame_id);
//             packets_buffers_[qid]->set_timestamp(packet_ptr, frame->timestamp);
//             packets_buffers_[qid]->write_complete();

//             //packet_count++;
//         }
//         //// for debug
//         //std::cout << "st2110 encode ns:" << timer.elapsed() << "  packet buffer size: " << packets_buffers_[qid]->get_buffer_size() << std::endl;
//         //std::cout << "packet count of per frame: " << packet_count << std::endl;
//         //logger->debug("process one frame(sequence number{}) need time:{} ms", sequence_number_, timer.elapsed());
//     }

//     /**
//      * @brief send the rtp packet by udp
//      * 
//      */
//     void rtp_st2110_output::send_packet(uint16_t queue_id)
//     {
//         auto id = queue_id;
//         udp_threads_.push_back(std::make_shared<std::thread>(std::thread([this, queue_id](){
//             //bind thread to one fixed cpu core
//             cpu_set_t mask;
//             CPU_ZERO(&mask);
//             CPU_SET(queue_id, &mask);
//             if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
//             {
//                 logger->error("bind udp thread to cpu failed");
//             }
            
//             uint64_t local_frame_time = 0;
//             uint32_t frame_id = 0;
//             uint64_t packet_time = 0;
//             while(!abort)
//             {
//                 try
//                 {
//                     //timer timer; // for debug
                    
//                     auto packet_idx = packets_buffers_[queue_id]->read();
//                     if(packet_idx != -1)
//                     {   
//                         uint8_t* packet_ptr = packets_buffers_[queue_id]->get_packet_ptr(packet_idx);
//                         // get frame start time and queue offset time(keep the send interval between the queues)
//                         auto id =  packets_buffers_[queue_id]->get_frame_id(packet_idx);
//                         if(frame_id != id) // new frame
//                         {
//                             frame_id = id;
//                             packet_time = timer::now() + drain_offsets_[queue_id]; //frame_time;
//                         }
                        
//                         auto elapse = timer::now() - packet_time;
//                         while(elapse < packet_drain_interval_) // wait until send time point arrive
//                         {
//                             elapse = timer::now() - packet_time;
//                         }
//                         packet_time = timer::now();

//                         // send data synchronously
//                         udp_senders_[0]->send_to(packet_ptr, 
//                                              packets_buffers_[queue_id]->get_packet_real_size(packet_idx), 
//                                              context_.multicast_ip, context_.multicast_port);
//                         packets_buffers_[queue_id]->read_complete();
                    
//                         //std::cout << "send udp packet: " << timer.elapsed() << std::endl;
//                     }
//                 }
//                 catch(const std::exception& e)
//                 {
//                     logger->error("send rtp packet to {}:{} error {}", context_.multicast_ip, context_.multicast_port, e.what());
//                 }
//             }
//         })));
//     }

//     /**
//      * @brief send the rtp packet by udp
//      * for debug
//      */
//     void rtp_st2110_output::send(uint16_t queue_id)
//     {
//         auto id = queue_id;
            
//         uint64_t local_frame_time = 0;
//         uint32_t frame_id = 0;
//         uint64_t packet_time = 0;
//         while(!abort)
//         {
//             try
//             {
//                 //timer timer; // for debug
                
//                 auto packet_idx = packets_buffers_[queue_id]->read();
//                 if(packet_idx != -1)
//                 {   
//                     uint8_t* packet_ptr = packets_buffers_[queue_id]->get_packet_ptr(packet_idx);
//                     // get frame start time and queue offset time(keep the send interval between the queues)
//                     auto id =  packets_buffers_[queue_id]->get_frame_id(packet_idx);
//                     if(frame_id != id) // new frame
//                     {
//                         frame_id = id;
//                         packet_time = timer::now() + drain_offsets_[queue_id]; //frame_time;
//                     }
                    
//                     auto elapse = timer::now() - packet_time;
//                     while(elapse < packet_drain_interval_) // wait until send time point arrive
//                     {
//                         elapse = timer::now() - packet_time;
//                     }
//                     packet_time = timer::now();

//                     // send data synchronously
//                     udp_senders_[0]->send_to(packet_ptr, 
//                                             packets_buffers_[queue_id]->get_packet_real_size(packet_idx), 
//                                             context_.multicast_ip, context_.multicast_port);
//                     packets_buffers_[queue_id]->read_complete();
                
//                     //std::cout << "send udp packet: " << timer.elapsed() << std::endl;
//                 }
//                 else
//                 {
//                     break;
//                 }
//             }
//             catch(const std::exception& e)
//             {
//                 logger->error("send rtp packet to {}:{} error {}", context_.multicast_ip, context_.multicast_port, e.what());
//             }
//         }
//     }

//     /**
//      * @brief push a frame into this output stream
//      * 
//      */
//     void rtp_st2110_output::set_frame(std::shared_ptr<core::frame> frm)
//     {
//         std::unique_lock<std::mutex> lock(frame_mutex_);
//         if(frame_buffer_.size() < frame_capacity_)
//         {
//             frame_buffer_.push_back(frm);
//             frame_cv_.notify_all();
//             return;
//         }
//         frame_cv_.wait(lock, [this](){return frame_buffer_.size() < frame_capacity_;}); // block until the buffer is not empty
//         frame_buffer_.push_back(frm);
//         frame_cv_.notify_all();
//         //logger->info("The frame buffer size is {}", frame_buffer_.size());

//         // std::lock_guard<std::mutex> lock(frame_mutex_);
//         // if(frame_buffer_.size() >= frame_capacity_)
//         // {
//         //     auto f = frame_buffer_[0];
//         //     frame_buffer_.pop_front(); // discard the last frame
//         //     logger->error("The frame is discarded");
//         // }
//         // frame_buffer_.push_back(frm);
//         // frame_cv_.notify_all();
//     }

//     /**
//      * @brief Get a frame from this output stream
//      * 
//      */
//     std::shared_ptr<core::frame> rtp_st2110_output::get_frame()
//     {
//         std::shared_ptr<core::frame> frm;
//         std::unique_lock<std::mutex> lock(frame_mutex_);
//         if(frame_buffer_.size() > 0)
//         {
//             frm = frame_buffer_[0];
//             frame_buffer_.pop_front();
//             frame_cv_.notify_all();
//             return frm;
//         }
//         frame_cv_.wait(lock, [this](){return !(frame_buffer_.empty());}); // block until the buffer is not empty
//         frm = frame_buffer_[0];
//         frame_buffer_.pop_front();
//         frame_cv_.notify_all();
//         return frm;
//     }

//     /**
//      * @brief Get the st2110 packet 
//      * 
//      * @return std::shared_ptr<packet> 
//      */
//     std::shared_ptr<packet> rtp_st2110_output::get_packet()
//     {
//         std::shared_ptr<rtp::packet> packet = nullptr;
//         // std::unique_lock<std::mutex> lock(packet_mutex_);
//         // if(packet_buffer_.size() > 0)
//         // {
//         //     packet = packet_buffer_[0];
//         //     packet_buffer_.pop_front();
//         //     packet_cv_.notify_all();
//         //     return packet;
//         // }
//         // packet_cv_.wait(lock, [this](){return !packet_buffer_.empty();});
//         // packet = packet_buffer_[0];
//         // packet_buffer_.pop_front();
//         // packet_cv_.notify_all();

//         // if(packet_buffer_.size() > 0)
//         // {
//         //     packet = packet_buffer_[0];
//         //     packet_buffer_.pop_front();
//         //     packet_cv_.notify_all();
//         // }

//         // boost lockfree spsc_queue
//         //auto ret = packet_bf_.pop(packet);
//         //if(!ret) logger->error("get packet failed");

//         return packet;
//     }

//     /**
//      * @brief Set the packet object
//      * 
//      */
//     void rtp_st2110_output::set_packet(std::shared_ptr<packet> packet)
//     {
//         // std::unique_lock<std::mutex> lock(packet_mutex_);
//         // if(packet_buffer_.size() < packet_capacity_)
//         // {
//         //     packet_buffer_.push_back(packet);
//         //     packet_cv_.notify_all();
//         //     return;
//         // }
//         // else{
//         //     packet.reset();
//         //     logger->error("packet was discarded");
//         //     return;
//         // }
//         // packet_cv_.wait(lock, [this](){return packet_buffer_.size() < packet_capacity_;});
//         // packet_buffer_.push_back(packet);
//         // packet_cv_.notify_all();

//         // boost lockfree spsc_queue
//         //auto ret = packet_bf_.push(packet);
//         //if(!ret) logger->error("packet was discarded");
//     }

//     /**
//      * @brief calculate rtp packets number per frame
//      * 
//      * @return int 
//      */
//     int rtp_st2110_output::calc_packets_per_frame(int line, int height)
//     {
//         int packets = 0;
//         //auto height = context_.format_desc.height;
//         auto width = context_.format_desc.width;
//         //int line = 0;
//         auto payload_size = d10::STANDARD_UDP_SIZE_LIMIT - sizeof(raw_header);
//         uint16_t pgroup = 5; // uyvy422 10bit, 5bytes
//         uint16_t xinc   = 2;
//         uint16_t yinc   = 1;
//         uint16_t offset = 0;

//         while(line < height)
//         {
//             auto left = payload_size;
//             uint32_t length, pixels;
//             uint8_t  continue_flag;
//             bool next_line = false;
//             uint8_t* headers;

//             left -= 2;
//             // while we can fit at least one header and pgroup
//             while (left > (6 + pgroup)) {
//                 left -= 6; // need a 6 bytes payload header(length,line no,offset)
//                 // get how many bytes we need for the remaining pixels
//                 pixels = width - offset;
//                 length = (pixels * pgroup) / xinc; // bytes,
//                 if (left >= length) {
//                     next_line = true;
//                 }
//                 else {
//                     pixels    = (left / pgroup) * xinc;
//                     length    = (pixels * pgroup) / xinc; //left;//pixels * pgroup / xinc;
//                     next_line = false;
//                 }
//                 left -= length;

//                 if (next_line)
//                     line += yinc;

//                 // calculate continuation marker
//                 continue_flag = (left > (6 + pgroup) && line < height) ? 0x80 : 0x00;

//                 if (next_line) { //new line, reset offset
//                     offset = 0; 
//                 } else {
//                     offset += pixels;
//                 }

//                 if (!continue_flag)
//                     break;
//             }

//             packets++;
//         }

//         return packets;
//     }
// }

// ////////////////////////////////////////////////send_packet backup
// //      /**
// //      * @brief send the rtp packet by udp
// //      * 
// //      */
// //     void rtp_st2110_output::send_packet()
// //     {
// //         udp_thread_ = std::make_unique<std::thread>(std::thread([&](){
// //             // bind thread to one fixed cpu core
// //             // cpu_set_t mask;
// //             // CPU_ZERO(&mask);
// //             // CPU_SET(23, &mask);
// //             // if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
// //             // {
// //             //     logger->error("bind udp thread to cpu failed");
// //             // }

// //             int timestamp = 0;
// //             frame_number_ = 0;
// //             packet_drained_number_ = 0;
// //             int64_t p_start_time = 0;
// //             int64_t frame_duration  = 1000000000 / context_.format_desc.fps; //nanosecond
// //             int64_t total_duration  = 0;
// //             int64_t packet_duration = 0;
// //             auto packet_drain_duration = static_cast<int64_t>((frame_duration / packets_per_frame_) / PACKET_DRAIN_RATE);
// //             while(!abort)
// //             {
// //                 try
// //                 {
// //                     auto packet = get_packet();
// //                     if(packet)
// //                     {
// //                         if(packet_drained_number_ == 0)
// //                         {
// //                             start_time_ = timer::now();
// //                             p_start_time = start_time_;
// //                         }
                        
// //                         if(packet->is_first_of_frame())
// //                         {
// //                             // wait until the frame time point arrive
// //                             auto played_time = timer::now() - start_time_;
// //                             total_duration = frame_duration * frame_number_;
// //                             while(played_time < total_duration) // wait until time point arrive
// //                             {
// //                                 played_time = timer::now() - start_time_;
// //                             }

// //                             auto time_now = date::now() + LEAP_SECONDS * 1000000; //microsecond. 37s: leap second, since 1970-01-01 to 2022-01-01
// //                             auto video_time = static_cast<uint64_t>(round(time_now * RTP_VIDEO_RATE / 1000000));
// //                             timestamp = static_cast<uint32_t>(video_time % RTP_WRAP_AROUND);
// //                             frame_number_++;
// //                         }

// //                         if(packet_drained_number_ == 0)
// //                         {
// //                             packet_duration = 0;
// //                         }
// //                         else
// //                         {
// //                             packet_duration = packet_drain_duration;
// //                         }

// //                         auto elapse = timer::now() - p_start_time;
// //                         while(elapse < packet_duration) // wait until time point arrive
// //                         {
// //                             elapse = timer::now() - p_start_time;
// //                         }
// //                         p_start_time = timer::now();
// //                         packet_drained_number_++;
// //                         packet->set_timestamp(timestamp);

// //                         // send data synchronously
// //                         // udp_sender_->send_to(packet->get_data_ptr(), packet->get_data_size(), 
// //                         //                      context_.multicast_ip, context_.multicast_port);

// //                         //send data asynchronously
// //                         // auto udp_sender = udp_senders_.at(sender_ptr_); // use multiple udp sender(socket) to accelerate send speed
// //                         // sender_ptr_++;
// //                         // if(sender_ptr_ >= udp_senders_.size()) sender_ptr_ = 0;
// //                         // auto f = [this, packet]() mutable {udp_sender_->send_to(packet->get_data_ptr(), 
// //                         //             packet->get_data_size(), context_.multicast_ip, context_.multicast_port); };
// //                         // executor_->execute(std::move(f));
// //                     }
// //                 }
// //                 catch(const std::exception& e)
// //                 {
// //                     logger->error("send rtp packet to {}:{} error {}", context_.multicast_ip, context_.multicast_port, e.what());
// //                 }
// //             }
// //         }));
// //     }