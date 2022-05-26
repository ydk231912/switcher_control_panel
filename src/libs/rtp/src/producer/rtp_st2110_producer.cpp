/**
 * @file rtp_st2110_procuder.cpp
 * @author 
 * @brief recieve st2110 udp packets, decode to video frame
 * @version 
 * @date 2022-05-24
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

#include "producer/rtp_st2110_producer.h"
#include "util/logger.h"
#include "util/timer.h"
#include "header.h"
#include "st2110/d20/raw_line_header.h"

using namespace seeder::util;
namespace seeder::rtp
{
    rtp_st2110_producer::rtp_st2110_producer(core::video_format_desc format_desc)
    :format_desc_(format_desc)
    {
        // create new frame
        current_frame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
    }

    rtp_st2110_producer::~rtp_st2110_producer()
    {
    }

    /**
     * @brief start producer:read packet from packet buffer, 
     * decode to frame, put the frame into frame buffer
     * 
     */
    void rtp_st2110_producer::run()
    {
        // util::timer timer;

        // 1. get packets
        std::unique_lock<std::mutex> lock(packet_mutex_);
        if(packet_buffer_.size() < 1) return;
        
        //cope packets pointer in local variable, in order to quickly unlock the packet buffer
        std::deque<std::shared_ptr<rtp::packet>> packets(packet_buffer_);
        packet_buffer_.clear();
        lock.unlock();

        // 2.decode packet
        for(auto packet : packets)
        {
            // 2.1 decode rtp header
            auto ptr = packet->get_data_ptr();
            auto rtp_header = reinterpret_cast<const raw_header*>(ptr);

            // csrc size
            constexpr auto csrc_size_bytes = 0x04;
            const auto csrc_size = csrc_size_bytes * rtp_header->csrc_count;
            // TODO: to support extensions
            if(rtp_header->extension)
            {
                logger->error("RTP Header Extension present cannot be process");
                return;
            }

            // detect frame transition
            if(last_frame_timestamp_ = 0)
            {
                last_frame_timestamp_ = rtp_header->timestamp;
            }
            if(last_frame_timestamp_ != rtp_header->timestamp)
            {
                // new frame, put current_frame to buffer, then alloc new frame
                send_frame(current_frame_);
                current_frame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
                last_frame_timestamp_ = rtp_header->timestamp;
            }

            // 2.2 decode line header
            ptr +=  sizeof(raw_header) + csrc_size;
            auto raw_extended_sequence = reinterpret_cast<const d20::raw_extended_sequence_number*>(ptr);
            auto sequence_number = (raw_extended_sequence->esn << 16) | rtp_header->sequence_number;
            ptr += sizeof(d20::raw_extended_sequence_number);

            int line_length[3] = {0, 0, 0}; //Only three lines per packet allowed by ST2110-20
            int line_number[3] = {0, 0, 0};
            int line_offset[3] = {0, 0, 0};
            int line_continuation[3] = {0, 0, 0};
            int line_field_identification[3] = {0, 0, 0};
            size_t line_index = 0;
            while(true)
            {
                auto line_header = reinterpret_cast<const d20::raw_line_header*>(ptr);
                ptr += sizeof(d20::raw_line_header);
                line_length[line_index] = line_header->length;
                line_number[line_index] = (line_header->line_no_1 << 8) | line_header->line_no_0;
                line_offset[line_index] = (line_header->offset_1 << 8) | line_header->offset_0;
                line_continuation[line_index] = line_header->continuation;
                line_field_identification[line_index] = line_header->field_identification;
                ++line_index;
                if(line_index > 3)
                {
                    logger->error("Only three lines per packet allowed by ST2110-20");
                    return;
                }
                if(!line_header->continuation) break;
            }

            // 2.3 process data segments
            for(int i = 0; i < line_index; i++)
            {
                const auto byte_offset = static_cast<int>(line_offset[i] * samples_per_pixel_ * color_depth_ / 8);
                const auto line_byte = static_cast<int>(format_desc_.width * samples_per_pixel_ * color_depth_ / 8) * line_number[i];
                const auto target = current_frame_->data[0] + line_byte + byte_offset;
                std::memcpy(target, ptr, line_length[i]);
                ptr += line_length[i];
            }
        }
        //util::logger->debug("process one frame(sequence number{}) need time:{} ms", sequence_number_, timer.elapsed());
    }

    /**
     * @brief send the frame to the rtp frame buffer 
     * 
     */
    void rtp_st2110_producer::send_frame(std::shared_ptr<AVFrame> frame)
    {
        std::unique_lock<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the last frame
        }
        frame_buffer_.push_back(frame);
    }

    /**
     * @brief Get the frame from the frame buffer
     * 
     */
    std::shared_ptr<AVFrame> rtp_st2110_producer::get_frame()
    {
        std::shared_ptr<AVFrame> frame;
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            if(frame_buffer_.size() > 0)
            {
                frame = frame_buffer_[0];
                frame_buffer_.pop_front();
            }            
        }

        return frame;
    }

    /**
     * @brief Get the st2110 packet 
     * 
     * @return std::shared_ptr<packet> 
     */
    std::shared_ptr<packet> rtp_st2110_producer::get_packet()
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

    /**
     * @brief Get the packet batch
     * 
     * @return std::deque<std::shared_ptr<rtp::packet>>
     */
    std::deque<std::shared_ptr<rtp::packet>> rtp_st2110_producer::get_packet_batch()
    {
        std::lock_guard<std::mutex> lock(packet_mutex_);        
        //cope packets pointer in local variable, in order to quickly unlock the packet buffer
        std::deque<std::shared_ptr<rtp::packet>> packets(packet_buffer_);
        packet_buffer_.clear();
        return packets;
    }

    void rtp_st2110_producer::send_packet(std::shared_ptr<packet> packet)
    {
        std::unique_lock<std::mutex> lock(packet_mutex_);
        if(packet_buffer_.size() < packet_capacity_)
        {
            packet_buffer_.push_back(packet); 
        } 
        else
        {
            // discard the last packet
            logger->error("The packet is discarded");
        }
    }

}