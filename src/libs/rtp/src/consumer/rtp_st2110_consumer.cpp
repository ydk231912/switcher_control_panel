/**
 * @file rtp_st2110_consumer.h
 * @author 
 * @brief recieve sdi frame, encodec to st2110 packet
 * @version 
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>
#include <exception>
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#include "consumer/rtp_st2110_consumer.h"
#include "util/logger.h"
#include "util/timer.h"

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
    void rtp_st2110_consumer::run()
    {
        //util::timer timer;

        //get frame from buffer
        core::frame frame;
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            frame_cv_.wait(lock, [this](){return !(frame_buffer_.empty());}); // block until the buffer is not empty

            frame = frame_buffer_[0];
            frame_buffer_.pop_front();
        }

        auto avframe = frame.video;
        if (!avframe)
            return;

        if (avframe->format != AV_PIX_FMT_YUV422P && avframe->format != AV_PIX_FMT_UYVY422)
        {
            //currently,only YUV422P UYVY422 format is supported
            util::logger->error("The {} frame format is unsupported!", avframe->format);
            throw std::runtime_error("The frame format is unsupported!");
        }

        uint16_t line   = 0;
        uint16_t pgroup = 5; // uyvy422 10bit, 5bytes
        uint16_t xinc   = 2;
        uint16_t yinc   = 1;
        uint16_t offset = 0;
        uint16_t width = avframe->width;
        height_ = avframe->height;
        int timestamp   = frame.timestamp; // get_timestamp();

        while(line < height_) {
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
                continue_flag = (left > (6 + pgroup) && line < height_) ? 0x80 : 0x00;
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
                uint32_t offs, ln, i, len, pgs;

                // read length and continue flag
                len = (headers[0] << 8) | headers[1];  // length
                ln    = ((headers[2] & 0x7F) << 8) | headers[3]; // line no
                offs   = ((headers[4] & 0x7F) << 8) | headers[5]; // offset
                continue_flag = headers[4] & 0x80;
                pgs           = len / pgroup; // how many pgroups
                headers += 6;

                switch (avframe->format) {
                    case AV_PIX_FMT_YUV422P:
                    {
                        // frame.data[n] 0:Y 1:U 2:V
                        uint8_t* yp   = avframe->data[0] + ln * width + offs;            // y point in frame
                        uint32_t temp     = (ln * width + offs) / xinc;
                        uint8_t* uvp1 = avframe->data[1] + temp; // uv point in frame
                        uint8_t* uvp2 = avframe->data[2] + temp;
                        // the graph is the Ycbcr 4:2:2 10bit depth, total 5bytes
                        // 0                   1                   2                   3
                        // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
                        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        // | C’B00 (10 bits)   | Y’00 (10 bits)    | C’R00 (10 bits)   | Y’01 (10 bits)   |
                        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        for (i = 0; i < pgs; i++) 
                        {
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
                        }
                        break;
                    }
                    case AV_PIX_FMT_UYVY422:
                    {
                        //packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
                        uint8_t* datap   = avframe->data[0] + ln * width + offs; 
                        for (i = 0; i < pgs; i++) 
                        {
                            //8bit UYVY(CbY0CrY1) to 10bit ycbcr(CbY0CrY1)
                            //      Cb         Y0        Cr         Y1
                            // xxxxxxxx 00++++++ ++00xxxx xxxx00++ ++++++00
                            //     0        1        2        3        4
                            *ptr++ = datap[0];
                            *ptr++ = datap[1] >> 2;
                            *ptr++ = datap[1] << 6 | datap[2] >> 4;
                            *ptr++ = datap[2] << 4 | datap[3] >> 6;
                            *ptr++ = datap[3] << 2;
                            datap+=4;
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
            rtp_packet->set_timestamp(timestamp);

            if (line >= height_) { //complete
                rtp_packet->set_marker(1);
            }

            if (left > 0) {
                rtp_packet->reset_size(rtp_packet->get_size() - left);
            }

            //// send packet by udp  //for debug test
            //client_.send_to(rtp_packet->get_data_ptr(), rtp_packet->get_data_size(), "239.0.10.1", 20000);
            // st20_to_png(rtp_packet);

            // 4 push to buffer
            std::unique_lock<std::mutex> lock(packet_mutex_);
            if(packet_buffer_.size() < packet_capacity_)
            {
                packet_buffer_.push_back(rtp_packet);
                packet_cv_.notify_all();

                //debug
                util::logger->debug("packet buffer size: {}", packet_buffer_.size());
            }
            else
            {
                util::logger->error("error: packet discarded! "); // discard
            }
        }

        //util::logger->debug("process one frame(sequence number{}) need time:{} ms", sequence_number_, timer.elapsed());
    }

    /**
     * @brief send frame to the rtp st2110 consumer 
     * if the frame buffer is full, dicard the last frame
     * 
     */
    void rtp_st2110_consumer::send_frame(core::frame frame)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the last frame
        }
        frame_buffer_.push_back(frame);
        frame_cv_.notify_all();
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
        packet_cv_.wait(lock, [this](){return !packet_buffer_.empty();});
        packet = packet_buffer_[0];
        packet_buffer_.pop_front();
        packet_cv_.notify_all();
        //util::logger->info("rtp_st2110::receive_packet: packet buffer size: {}", packet_buffer_.size());

        return packet;
    }

    /**
     * @brief Get the packet batch
     * 
     * @return std::vector<std::shared_ptr<packet>> 
     */
    std::vector<std::shared_ptr<packet>> rtp_st2110_consumer::get_packet_batch()
    {
        std::vector<std::shared_ptr<rtp::packet>> packets;
        int size = 1;
        if(height_ > 0)
        {
            size = int(height_ / chunks_per_frame_); // each frame send chunks_per_frame_ times
        }

        std::unique_lock<std::mutex> lock(packet_mutex_);
        packet_cv_.wait(lock, [this](){return !packet_buffer_.empty();});
        if(size > packet_buffer_.size())
        {
            size = packet_buffer_.size();
        }

        packets.reserve(size);
        for(int i = 0; i < size; i++)
        {
            std::shared_ptr<rtp::packet> p = packet_buffer_[0];
            packet_buffer_.pop_front();
            packets.push_back(p);
            //util::logger->info("rtp_st2110::receive_packet: packet buffer size: {}", packet_buffer_.size());
        }
        packet_cv_.notify_all();

        return packets;
    }

}