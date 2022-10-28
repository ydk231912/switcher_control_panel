/**
 * @file rtp_st2110_output.cpp
 * @author 
 * @brief recieve frame, encodec to st2110 packet, send packet by udp
 * @version 1.0
 * @date 2022-07-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <iostream>
#include <exception>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <unistd.h>

extern "C"
{
    #include <libavformat/avformat.h>
}

#include "core/util/logger.h"
#include "core/util/timer.h"
#include "core/util/date.h"
#include "st2110/d20/raw_line_header.h"
#include "st2110/d10/network.h"
#include "rtp/header.h"
#include "rtp/rtp_constant.h"
#include "rtp/util.h"

#include "output/rtp_st2110_output_bk.h"

using namespace seeder::core;
namespace seeder::rtp
{
    rtp_st2110_output_bk::rtp_st2110_output_bk(rtp_context& context)
    :context_(context)
    ,udp_sender_(std::make_unique<net::udp_sender>())
    ,executor_(std::make_unique<core::executor>())
    {
        //udp_sender_->bind_ip("192.168.10.203", 20001);
        packets_per_frame_ = calc_packets_per_frame();
        packet_capacity_ =  packets_per_frame_ * frame_capacity_ ;

        frame_period_ = static_cast<uint32_t>(1 / context_.format_desc.fps * RTP_VIDEO_RATE);
    }

    rtp_st2110_output_bk::~rtp_st2110_output_bk()
    {
    }

    /**
     * @brief start output stream handle
     * 
     */
    void rtp_st2110_output_bk::start()
    {
        abort = false;
        encode();
        send_packet();
    }
        
    /**
     * @brief stop output stream handle
     * 
     */
    void rtp_st2110_output_bk::stop()
    {
        abort = true;
    }

    /**
     * @brief encode the frame data into st2110 rtp packet
     * 
     */
    void rtp_st2110_output_bk::encode()
    {
        st2110_thread_ = std::make_unique<std::thread>(std::thread([&](){
            while(!abort) 
            {
                // bind thread to one fixed cpu core
                // cpu_set_t mask;
                // CPU_ZERO(&mask);
                // CPU_SET(23, &mask);
                // if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
                // {
                //     logger->error("bind udp thread to cpu failed");
                // }
                do_encode();
            }
        }));
    }

    void rtp_st2110_output_bk::do_encode()
    {
        //get frame from buffer
        auto frame = get_frame();

        timer timer; // for debug

        if (frame->video->format != AV_PIX_FMT_YUV422P && frame->video->format != AV_PIX_FMT_UYVY422)
        {
            //currently,only YUV422P UYVY422 format is supported
            logger->error("The {} frame format is unsupported!", frame->video->format);
            throw std::runtime_error("The frame format is unsupported!");
        }

        int packet_count = 0;
        bool first_of_frame = true;

        uint16_t line   = 0;
        uint16_t pgroup = 5; //5; uyvy422 10bit, 5bytes; 4 uyvy422 8bit
        uint16_t xinc   = 2;
        uint16_t yinc   = 1;
        uint16_t offset = 0;
        uint16_t width = frame->video->width;
        height_ = frame->video->height;
        //auto timestamp   = frame->timestamp;

        while(line < height_) {
            uint32_t left, length, pixels;
            uint8_t  continue_flag;
            bool next_line = false;
            uint8_t* headers;

            std::shared_ptr<rtp::packet> rtp_packet = std::make_shared<rtp::packet>();
            if(first_of_frame)
            {
                rtp_packet->set_first_of_frame(first_of_frame);
                first_of_frame = false;
            }
            uint8_t* ptr = rtp_packet->get_payload_ptr();
            left = rtp_packet->get_payload_size();

            //RTP Payload Header
            // from https://tools.ietf.org/html/rfc4175
            // 0                               16                             32
            // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            // |   Extended Sequence Number    |            Length             |
            // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            // |F|          Line No            |C|           Offset            |
            // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            // |            Length             |F|          Line No            |
            // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            // |C|           Offset            |                               .
            // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               .
            // .                                                               .
            // .                 Two (partial) lines of video data             .
            // .                                                               .
            // +---------------------------------------------------------------+

            ptr[0] = 0;  // extender sequence number 16bit. same as: *ptr = 0; ptr = ptr + 1;
            ptr[1] = 0;
            ptr += 2;
            left -= 2;
            headers = ptr;
            

            //// 1 write payload header
            // while we can fit at least one header and pgroup
            while (left > (6 + pgroup)) {
                left -= 6; // need a 6 bytes payload header(length,line no,offset)

                // get how many bytes we need for the remaining pixels
                pixels = width - offset;
                length = (pixels * pgroup) / xinc; // bytes,
                if (left >= length) {
                    next_line = true;
                }
                else {
                    pixels    = (left / pgroup) * xinc;
                    length    = (pixels * pgroup) / xinc; //left;//pixels * pgroup / xinc;
                    next_line = false;
                }
                left -= length;

                // write length and line number in payload header
                ptr[0]   = (length >> 8) & 0xFF; // big-endian
                ptr[1] = length & 0xFF;
                ptr[2] = ((line >> 8) & 0x7F) | ((0 << 7) & 0x80); //line no 7bit and "F" flag 1bit
                ptr[3] = line & 0xFF;

                if (next_line)
                    line += yinc;

                // calculate continuation marker
                continue_flag = (left > (6 + pgroup) && line < height_) ? 0x80 : 0x00;
                // write offset and continuation marker in header
                ptr[4] = ((offset >> 8) & 0x7F) | continue_flag;
                ptr[5] = offset & 0xFF;
                ptr += 6;

                if (next_line) { //new line, reset offset
                    offset = 0; 
                } else {
                    offset += pixels;
                }

                if (!continue_flag)
                    break;
            }

            //// 2 write data in playload
            while (true) {
                uint32_t offs, ln, i, len, pgs;

                // read length and continue flag
                len = (headers[0] << 8) | headers[1];  // length
                ln    = ((headers[2] & 0x7F) << 8) | headers[3]; // line no
                offs   = ((headers[4] & 0x7F) << 8) | headers[5]; // offset
                continue_flag = headers[4] & 0x80;
                pgs           = len / pgroup; // how many pgroups
                headers += 6;

                switch (frame->video->format) {
                    case AV_PIX_FMT_YUV422P:
                    {
                        // frame.data[n] 0:Y 1:U 2:V
                        uint8_t* yp   = frame->video->data[0] + ln * width + offs;            // y point in frame
                        uint32_t temp     = (ln * width + offs) / xinc;
                        uint8_t* uvp1 = frame->video->data[1] + temp; // uv point in frame
                        uint8_t* uvp2 = frame->video->data[2] + temp;
                        // the graph is the Ycbcr 4:2:2 10bit depth, total 5bytes
                        // 0                   1                   2                   3
                        // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
                        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        // | C’B00 (10 bits)   | Y’00 (10 bits)    | C’R00 (10 bits)   | Y’01 (10 bits)   |
                        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        for (i = 0; i < pgs; i++) 
                        {
                            ////////////////// 10 bit
                            // 8bit yuv to 10bit ycbcr(CbY0CrY1)
                            //      Cb         Y0        Cr         Y1
                            // xxxxxxxx 00++++++ ++00xxxx xxxx00++ ++++++00
                            //     0        1        2        3        4
                            *ptr++ = *uvp1;
                            *ptr++ = *yp >> 2;               //(*yp & 0xFC) >> 2;
                            *ptr++ = *yp << 6 | *uvp2 >> 4;  // ((*yp & 0x03) << 6) | ((*uvp2 & 0xF0) >> 4);
                            ++yp;
                            *ptr++ = *uvp2 << 4 | *yp >> 6;  //((*uvp2 & 0x0F) << 4) | ((*yp & 0xC0) >> 6);
                            *ptr++ = *yp << 2;               //(*yp & 0x3F) << 2;

                            ++uvp1;
                            ++yp;
                            ++uvp2;

                            ///////////////// 8 bit
                            // 8bit yuv to 8bit ycbcr(CbY0CrY1)
                            //    Cb       Y0        Cr      Y1
                            // xxxxxxxx ++++++++ xxxxxxxx ++++++++
                            //     0        1        2        3 
                            // ptr[0] = uvp1[0];
                            // ptr[1] = yp[0];
                            // ptr[2] = uvp2[0];
                            // ptr[3] = yp[1];

                            // ++uvp1;
                            // ++uvp2;
                            // yp += 2;
                            // ptr +=4;
                        }
                        break;
                    }
                    case AV_PIX_FMT_UYVY422:
                    {
                        //packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
                        uint8_t* datap   = frame->video->data[0] + ln * width + offs; 
                        for (i = 0; i < pgs; i++) 
                        {
                            //8bit UYVY(CbY0CrY1) to 10bit ycbcr(CbY0CrY1)
                            //      Cb         Y0        Cr         Y1
                            // xxxxxxxx 00++++++ ++00xxxx xxxx00++ ++++++00
                            //     0        1        2        3        4
                            // *ptr++ = datap[0];
                            // *ptr++ = datap[1] >> 2;
                            // *ptr++ = datap[1] << 6 | datap[2] >> 4;
                            // *ptr++ = datap[2] << 4 | datap[3] >> 6;
                            // *ptr++ = datap[3] << 2;
                            ptr[0] = datap[0];
                            ptr[1] = datap[1] >> 2;
                            ptr[2] = datap[1] << 6 | datap[2] >> 4;
                            ptr[3] = datap[2] << 4 | datap[3] >> 6;
                            ptr[4] = datap[3] << 2;
                            datap += 4;
                            ptr += 5;
                        }
                        break;
                    }
                }

                if (!continue_flag)
                    break;
            }

            //// 3 write rtp header
            sequence_number_++;
            rtp_packet->set_sequence_number(sequence_number_);
            //rtp_packet->set_timestamp(timestamp);

            if (line >= height_) { //complete
                rtp_packet->set_marker(1);
            } 

            if (left > 0) {
                rtp_packet->reset_size(rtp_packet->get_size() - left);
            }

            // 4 push to buffer
            set_packet(rtp_packet);
            //packet_count++;
        }
        //// for debug
        //std::cout << "st2110 encode ns:" << timer.elapsed() << std::endl;
        //std::cout << "packet count of per frame: " << packet_count << std::endl;
        //logger->debug("process one frame(sequence number{}) need time:{} ms", sequence_number_, timer.elapsed());
    }

    /**
     * @brief send the rtp packet by udp
     * 
     */
    void rtp_st2110_output_bk::send_packet()
    {
        udp_thread_ = std::make_unique<std::thread>(std::thread([&](){
            // bind thread to one fixed cpu core
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(23, &mask);
            if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
            {
                logger->error("bind udp thread to cpu failed");
            }

            uint32_t timestamp = 0;
            //int timestamp = 0;
            frame_number_ = 0;
            packet_drained_number_ = 0;
            int64_t p_start_time = 0;
            int64_t frame_duration  = 1000000000 / context_.format_desc.fps; //nanosecond
            int64_t total_duration  = 0;
            int64_t packet_duration = 0;
            auto packet_drain_duration = static_cast<int64_t>((frame_duration / packets_per_frame_) / PACKET_DRAIN_RATE);
            while(!abort)
            {
                try
                {
                    auto packet = get_packet();
                    if(packet)
                    {
                        if(packet_drained_number_ == 0)
                        {
                            start_time_ = timer::now();
                            p_start_time = start_time_;
                        }
                        
                        if(packet->is_first_of_frame())
                        {
                            // wait until the frame time point arrive
                            int64_t played_time = timer::now() - start_time_;
                            total_duration = frame_duration * frame_number_;
                            while(played_time < total_duration) // wait until time point arrive
                            {
                                played_time = timer::now() - start_time_;
                            }

                            // auto time_now = date::now() + LEAP_SECONDS * 1000000; //microsecond. 37s: leap second, since 1970-01-01 to 2022-01-01
                            // auto video_time = static_cast<uint64_t>(round(time_now * RTP_VIDEO_RATE / 1000000));
                            // timestamp = static_cast<uint32_t>(video_time % RTP_WRAP_AROUND);

                            // auto time_now = date::now() ;//+ LEAP_SECONDS * 1000000; //microsecond. 37s: leap second, since 1970-01-01 to 2022-01-01
                            // auto video_time = static_cast<uint64_t>(round(time_now * 0.09));  //(RTP_VIDEO_RATE / 1000000)));
                            // timestamp = static_cast<uint32_t>(video_time % RTP_WRAP_AROUND);
                            if(timestamp == 0)
                            {
                                timestamp = get_video_timestamp(context_.leap_seconds);
                            }
                            else
                            {
                                timestamp += frame_period_;
                            }
                            
                            frame_number_++;
                        }

                        if(packet_drained_number_ == 0)
                        {
                            packet_duration = 0;
                        }
                        else
                        {
                            packet_duration = packet_drain_duration;
                        }

                        int64_t elapse = timer::now() - p_start_time;
                        while(elapse < packet_duration) // wait until time point arrive
                        {
                            elapse = timer::now() - p_start_time;
                        }
                        p_start_time = timer::now();
                        packet_drained_number_++;
                        packet->set_timestamp(timestamp);

                        // send data synchronously
                        //auto send_time = timer::now(); 
                        udp_sender_->send_to(packet->get_data_ptr(), packet->get_data_size(), 
                                             context_.multicast_ip, context_.multicast_port);
                        // auto send_elapse = timer::now() - send_time;
                        // if(send_elapse > 5000 && packet_drained_number_ < 40000) std::cout << "packets: " << packet_drained_number_<< " udp send time spend: " << send_elapse << std::endl;

                        //send data asynchronously
                        // auto udp_sender = udp_senders_.at(sender_ptr_); // use multiple udp sender(socket) to accelerate send speed
                        // sender_ptr_++;
                        // if(sender_ptr_ >= udp_senders_.size()) sender_ptr_ = 0;
                        // auto f = [this, packet]() mutable {udp_sender_->send_to(packet->get_data_ptr(), 
                        //             packet->get_data_size(), context_.multicast_ip, context_.multicast_port); };
                        // executor_->execute(std::move(f));
                    }
                }
                catch(const std::exception& e)
                {
                    logger->error("send rtp packet to {}:{} error {}", context_.multicast_ip, context_.multicast_port, e.what());
                }
            }
        }));
    }

    /**
     * @brief push a frame into this output stream
     * 
     */
    void rtp_st2110_output_bk::set_frame(std::shared_ptr<core::frame> frm)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() < frame_capacity_)
        {
            frame_buffer_.push_back(frm);
            frame_cv_.notify_all();
            return;
        }
        frame_cv_.wait(lock, [this](){return frame_buffer_.size() < frame_capacity_;}); // block until the buffer is not empty
        frame_buffer_.push_back(frm);
        frame_cv_.notify_all();
        //logger->info("The frame buffer size is {}", frame_buffer_.size());

        // std::lock_guard<std::mutex> lock(frame_mutex_);
        // if(frame_buffer_.size() >= frame_capacity_)
        // {
        //     auto f = frame_buffer_[0];
        //     frame_buffer_.pop_front(); // discard the last frame
        //     logger->error("The frame is discarded");
        // }
        // frame_buffer_.push_back(frm);
        // frame_cv_.notify_all();
    }

    /**
     * @brief Get a frame from this output stream
     * 
     */
    std::shared_ptr<core::frame> rtp_st2110_output_bk::get_frame()
    {
        std::shared_ptr<core::frame> frm;
        std::unique_lock<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() > 0)
        {
            frm = frame_buffer_[0];
            frame_buffer_.pop_front();
            frame_cv_.notify_all();
            return frm;
        }
        frame_cv_.wait(lock, [this](){return !(frame_buffer_.empty());}); // block until the buffer is not empty
        frm = frame_buffer_[0];
        frame_buffer_.pop_front();
        frame_cv_.notify_all();
        return frm;
    }

    /**
     * @brief Get the st2110 packet 
     * 
     * @return std::shared_ptr<packet> 
     */
    std::shared_ptr<packet> rtp_st2110_output_bk::get_packet()
    {
        std::shared_ptr<rtp::packet> packet = nullptr;
        std::unique_lock<std::mutex> lock(packet_mutex_);
        if(packet_buffer_.size() > 0)
        {
            packet = packet_buffer_[0];
            packet_buffer_.pop_front();
            packet_cv_.notify_all();
            return packet;
        }
        packet_cv_.wait(lock, [this](){return !packet_buffer_.empty();});
        packet = packet_buffer_[0];
        packet_buffer_.pop_front();
        packet_cv_.notify_all();

        // if(packet_buffer_.size() > 0)
        // {
        //     packet = packet_buffer_[0];
        //     packet_buffer_.pop_front();
        //     packet_cv_.notify_all();
        // }

        ///////// boost lockfree spsc_queue
        //auto ret = packet_bf_.pop(packet);
        //if(!ret) logger->error("get packet failed");

        return packet;
    }

    /**
     * @brief Set the packet object
     * 
     */
    void rtp_st2110_output_bk::set_packet(std::shared_ptr<packet> packet)
    {
        std::unique_lock<std::mutex> lock(packet_mutex_);
        if(packet_buffer_.size() < packet_capacity_)
        {
            packet_buffer_.push_back(packet);
            packet_cv_.notify_all();
            return;
        }
        // else{
        //     packet.reset();
        //     logger->error("packet was discarded");
        //     return;
        // }
        packet_cv_.wait(lock, [this](){return packet_buffer_.size() < packet_capacity_;});
        packet_buffer_.push_back(packet);
        packet_cv_.notify_all();

        /////// boost lockfree spsc_queue
        //auto ret = packet_bf_.push(packet);
        //if(!ret) logger->error("packet was discarded");
    }

    /**
     * @brief calculate rtp packets number per frame
     * 
     * @return int 
     */
    int rtp_st2110_output_bk::calc_packets_per_frame()
    {
        int packets = 0;
        auto height = context_.format_desc.height;
        auto width = context_.format_desc.width;
        int line = 0;
        auto payload_size = d10::STANDARD_UDP_SIZE_LIMIT - sizeof(raw_header);
        uint16_t pgroup = 5; // 5 uyvy422 10bit, 5bytes, 4 8bit
        uint16_t xinc   = 2;
        uint16_t yinc   = 1;
        uint16_t offset = 0;

        while(line < height)
        {
            auto left = payload_size;
            uint32_t length, pixels;
            uint8_t  continue_flag;
            bool next_line = false;
            uint8_t* headers;

            left -= 2;
            // while we can fit at least one header and pgroup
            while (left > (6 + pgroup)) {
                left -= 6; // need a 6 bytes payload header(length,line no,offset)
                // get how many bytes we need for the remaining pixels
                pixels = width - offset;
                length = (pixels * pgroup) / xinc; // bytes,
                if (left >= length) {
                    next_line = true;
                }
                else {
                    pixels    = (left / pgroup) * xinc;
                    length    = left; //left;//pixels * pgroup / xinc;
                    next_line = false;
                }
                left -= length;

                if (next_line)
                    line += yinc;

                // calculate continuation marker
                continue_flag = (left > (6 + pgroup) && line < height) ? 0x80 : 0x00;

                if (next_line) { //new line, reset offset
                    offset = 0; 
                } else {
                    offset += pixels;
                }

                if (!continue_flag)
                    break;
            }

            packets++;
        }

        return packets;
    }
}