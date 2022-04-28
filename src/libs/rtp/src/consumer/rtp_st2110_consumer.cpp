/**
 * @file rtp_st2110_consumer.h
 * @author your name (you@domain.com)
 * @brief recieve sdi frame, encodec to st2110 packet
 * @version 0.1
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#include "consumer/rtp_st2110_consumer.h"
#include "util/logger.h"

namespace seeder::rtp
{
    rtp_st2110_consumer::rtp_st2110_consumer()
    {
        sequence_number_ = 0;
    }

    rtp_st2110_consumer::~rtp_st2110_consumer()
    {
    }

    /**
     * @brief start consumer: read frame from the frame buffer, 
     * encode to st2110 udp packet, put the packet into packet buffer
     * 
     */
    void rtp_st2110_consumer::start()
    {
        //auto s = boost::posix_time::microsec_clock::local_time();

        //get frame from buffer
        util::frame f;
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            if(frame_buffer_.size() < 1)
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
                return;
            }

            f = frame_buffer_[0];
            frame_buffer_.pop_front();
        }

        auto frame = f.video;
        if (!frame)
            return;

        if (frame->format != AV_PIX_FMT_YUV422P)
            return; //currently,only YUV422P format is supported

        uint16_t width, height;
        uint16_t line   = 0;
        uint16_t pgroup = 5; // uyvy422 10bit, 5bytes
        uint16_t xinc   = 2;
        uint16_t yinc   = 1;
        uint16_t offset = 0;
        width = frame->width;
        height = frame->height;
        int timestamp   = f.timestamp; // get_timestamp();

        while(line < height) {
            if(line >= 100)
            {
                int i = line;
            }
            uint32_t left, length, pixels;
            uint8_t  continue_flag;
            bool next_line = false;
            uint8_t* headers;

            std::shared_ptr<rtp::packet> rtp_packet = std::make_shared<rtp::packet>();
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

            *ptr++ = 0;  // extender sequence number 16bit. same as: *ptr = 0; ptr = ptr + 1;
            *ptr++ = 0;
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
                    length    = left; //left;//pixels * pgroup / xinc;
                    next_line = false;
                }
                left -= length;

                // write length and line no in payload header
                *ptr++   = (length >> 8) & 0xFF; // big-endian
                *ptr++ = length & 0xFF;
                *ptr++ = ((line >> 8) & 0x7F) | ((0 << 7) & 0x80); //line no 7bit and "F" flag 1bit
                *ptr++ = line & 0xFF;

                if (next_line)
                    line += yinc;

                // calculate continuation marker
                continue_flag = (left > (6 + pgroup) && line < height) ? 0x80 : 0x00;
                // write offset and continuation marker in header
                *ptr++ = ((offset >> 8) & 0x7F) | continue_flag;
                *ptr++ = offset & 0xFF;

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
                uint32_t offs, ln, i, len, pgs, p;
                uint8_t *yp, *uvp1, *uvp2;

                // read length and continue flag
                len = (headers[0] << 8) | headers[1];  // length
                ln    = ((headers[2] & 0x7F) << 8) | headers[3]; // line no
                offs   = ((headers[4] & 0x7F) << 8) | headers[5]; // offset
                continue_flag = headers[4] & 0x80;
                pgs           = len / pgroup; // how many pgroups
                headers += 6;
                yp   = frame->data[0] + ln * width + offs;            // y point in frame
                p     = (ln * width + offs) / xinc;
                uvp1 = frame->data[1] + p; // uv point in frame
                uvp2 = frame->data[2] + p;

                switch (frame->format) {
                    case AV_PIX_FMT_YUV422P:
                        // the graph is the Ycbcr 4:2:2 10bit depth, total 5bytes
                        // 0                   1                   2                   3
                        // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
                        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        // | C’B00 (10 bits)   | Y’00 (10 bits)    | C’R00 (10 bits)   | Y’01 (10 bits)   |
                        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        // frame.data[n] 0:Y 1:U 2:V
                        for (i = 0; i < pgs; i++) {
                            // 8bit yuv to 10bit ycbcr
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
                        }

                        break;
                }

                if (!continue_flag)
                    break;
            }

            //// 3 write rtp header
            sequence_number_++;
            rtp_packet->set_sequence_number(sequence_number_);
            rtp_packet->set_timestamp(timestamp);

            if (line >= height) { //complete
                rtp_packet->set_marker(1);
            }

            if (left > 0) {
                rtp_packet->reset_size(rtp_packet->get_size() - left);
            }

            //// send packet by udp  //for debug test
            //client_.send_to(rtp_packet->get_data_ptr(), rtp_packet->get_data_size(), "239.0.10.1", 20000);
            // st20_to_png(rtp_packet);

            // 4 push to buffer  
            if(packet_buffer_.size() < packet_capacity_)
            {
                std::unique_lock<std::mutex> lock(packet_mutex_);
                packet_buffer_.push_back(rtp_packet);
                //debug
                util::logger->debug("packet buffer size: {}", packet_buffer_.size());
            }
            else
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
                util::logger->info("error: packet discarded! "); // discard
            }
        }

        //auto e = boost::posix_time::microsec_clock::local_time();
        //util::logger->debug("start st2110 consumer send begin:{}", boost::posix_time::to_iso_string(e - s));
    }

    /**
     * @brief send frame to the rtp st2110 consumer 
     * when the frame buffer is full, dicard the oldest frame
     * 
     */
    void rtp_st2110_consumer::send_frame(util::frame frame)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the oldest frame
        }
        frame_buffer_.push_back(frame);
    }

    /**
     * @brief Get the st2110 packet 
     * 
     * @return std::shared_ptr<packet> 
     */
    std::shared_ptr<packet> rtp_st2110_consumer::get_packet()
    {
        std::shared_ptr<rtp::packet> packet = nullptr;
        std::unique_lock<std::mutex> lock(packet_mutex_);
        if(packet_buffer_.size() > 0)
        {
            packet = packet_buffer_[0];
            packet_buffer_.pop_front();
            //util::logger->info("rtp_st2110::receive_packet: packet buffer size: {}", packet_buffer_.size());
        }

        return packet;
    }

}