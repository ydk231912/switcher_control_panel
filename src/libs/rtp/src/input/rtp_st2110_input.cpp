/**
 * @file rtp_st2110_input.cpp
 * @author 
 * @brief recieve st2110 udp packets, decode to video frame
 * @version 
 * @date 2022-07-24
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

#include "input/rtp_st2110_input.h"
#include "core/util/logger.h"
#include "core/util/timer.h"
#include "header.h"
#include "st2110/d20/raw_line_header.h"

using namespace seeder::core;
namespace seeder::rtp
{
    rtp_st2110_input::rtp_st2110_input(rtp_context context)
    :context_(context),
    receiver_(context.multicast_ip)
    {
        // create new frame
        frame_size_ = static_cast<int>((context_.format_desc.width * 5 / 2) * context_.format_desc.height); // 2 pixel = 40bit
        // buffer_ = std::make_shared<buffer>(frame_size_);
        // current_frame_ = std::shared_ptr<core::frame>(new core::frame(), [&](core::frame* ptr){ buffer_->free(); });
        // current_frame_->video_data[0] = buffer_->begin();
    }

    rtp_st2110_input::~rtp_st2110_input()
    {
    }

    /**
     * @brief start input:read packet from packet buffer, 
     * decode to frame, put the frame into frame buffer
     * 
     */
    void rtp_st2110_input::start()
    {
        abort = true;

        receive_packet();

        st2110_thread_ =std::make_unique<std::thread>(std::thread([&](){
            while(!abort)
            {
                decode();
            }
        }));
    }

    void rtp_st2110_input::stop()
    {
        abort = false;
    }

    /**
     * @brief run input:read packet from packet buffer, 
     * decode to frame, put the frame into frame buffer
     * 
     */
    void rtp_st2110_input::decode()
    {
        // timer timer;

        // 1. get packets
        std::unique_lock<std::mutex> lock(packet_mutex_);
        packet_cv_.wait(lock, [this](){return !packet_buffer_.empty();});
        
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
                buffer_ = (uint8_t*)malloc(frame_size_);
            }
            if(last_frame_timestamp_ != rtp_header->timestamp)
            {
                // new frame, put current_frame to buffer, then alloc new frame
                auto frm = std::shared_ptr<core::frame>(new core::frame(), [](core::frame* ptr){ free(ptr->video_data[0]); });
                frm->video_data[0] = buffer_;
                set_frame(frm);
                buffer_ = (uint8_t*)malloc(frame_size_);
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
                const auto line_byte = static_cast<int>(context_.format_desc.width * samples_per_pixel_ * color_depth_ / 8) * line_number[i];
                const auto target = buffer_ + line_byte + byte_offset;
                // TODO: 10bit UYVY to 8bit UYVY, because ffmpeg not have 10bit UYVY format
                std::memcpy(target, ptr, line_length[i]);
                ptr += line_length[i];
            }
        }
        //logger->debug("process one frame(sequence number{}) need time:{} ms", sequence_number_, timer.elapsed());
    }

    /**
     * @brief send the frame to the rtp frame buffer 
     * 
     */
    void rtp_st2110_input::set_frame(std::shared_ptr<core::frame> frm)
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the last frame
            logger->error("The frame is discarded");
        }
        frame_buffer_.push_back(frm);
    }

    /**
     * @brief Get the frame from the frame buffer
     * 
     */
    std::shared_ptr<core::frame> rtp_st2110_input::get_frame()
    {
        std::shared_ptr<core::frame> frm;
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            if(frame_buffer_.size() > 0)
            {
                frm = frame_buffer_[0];
                frame_buffer_.pop_front();
            }            
        }

        return frm;
    }

    /**
     * @brief Get the st2110 packet 
     * 
     * @return std::shared_ptr<packet> 
     */
    std::shared_ptr<packet> rtp_st2110_input::get_packet()
    {
        std::shared_ptr<rtp::packet> packet = nullptr;
        std::lock_guard<std::mutex> lock(packet_mutex_);
        if(packet_buffer_.size() > 0)
        {
            packet = packet_buffer_[0];
            packet_buffer_.pop_front();
            //logger->info("rtp_st2110::receive_packet: packet buffer size: {}", packet_buffer_.size());
        }

        return packet;
    }

    /**
     * @brief Get the packet batch
     * 
     * @return std::deque<std::shared_ptr<rtp::packet>>
     */
    std::deque<std::shared_ptr<rtp::packet>> rtp_st2110_input::get_packet_batch()
    {
        std::lock_guard<std::mutex> lock(packet_mutex_);        
        //cope packets pointer in local variable, in order to quickly unlock the packet buffer
        std::deque<std::shared_ptr<rtp::packet>> packets(packet_buffer_);
        packet_buffer_.clear();
        return packets;
    }

    /**
     * @brief add packet to input stream buffer
     * 
     * @param packet 
     */
    void rtp_st2110_input::set_packet(std::shared_ptr<packet> packet)
    {
        std::lock_guard<std::mutex> lock(packet_mutex_);
        if(packet_buffer_.size() < packet_capacity_)
        {
            packet_buffer_.push_back(packet); 
            packet_cv_.notify_all();
        } 
        else
        {
            // discard the last packet
            logger->error("The packet is discarded");
        }
    }

    /**
     * @brief receive packet by udp multicast
     * 
     */
    void rtp_st2110_input::receive_packet()
    {
        udp_thread_ = std::make_unique<std::thread>(std::thread([&](){
            while(!abort)
            {
                auto packet = std::make_shared<rtp::packet>(1900);
                auto len = receiver_.receive(packet->get_data_ptr(), packet->get_size());
                if(len > 0)
                    set_packet(packet);
            }
        }));
    }

}