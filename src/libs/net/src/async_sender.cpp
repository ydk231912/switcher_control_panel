/**
 * udp client by linux native socket
 *
 */

#include <iostream>

#include "async_sender.h"

namespace seeder { namespace net {
    
    async_sender::async_sender(uint32_t capacity)
    :capacity_(capacity)
    {
        init();
    }

    async_sender::async_sender()
    :capacity_(100000)
    {
        init();
    }

    async_sender::~async_sender(){}

    void async_sender::init()
    {
        buffer_ = std::vector<data_info>(capacity_);
        in_ = 0;
        out_ = 0;
        size_ = 0;
        
        udp_sender_ = std::make_shared<net::udp_sender>();
    }

    /**
     * @brief loop read the data from buffer, send by udp
     * 
     */
    void async_sender::start()
    {
        abort_ = false;
        thread_ = std::thread([&](){
            while(!abort_){
                int index = -1;
                index = read();
                while(index == -1)
                {
                    index = read();
                }

                udp_sender_->send_to(buffer_[index].data, buffer_[index].length,
                        buffer_[index].ip, buffer_[index].port);

                // if(buffer_[index].callback)
                //     buffer_[index].callback();

                //std::cout << "udp buffer size:  " << size_ << std::endl;

                size_--;                
            }
        });
    }

    void async_sender::start2()
    {
        abort_ = false;
        thread_ = std::thread([&](){
            while(!abort_){
                std::shared_ptr<data_info> packet = nullptr;
                std::unique_lock<std::mutex> lock(packet_mutex_);
                if(packet_buffer_.size() > 0)
                {
                    packet = packet_buffer_[0];
                    packet_buffer_.pop_front();
                    packet_cv_.notify_all();
                }
                else
                {
                    packet_cv_.wait(lock, [this](){return !packet_buffer_.empty();});
                    packet = packet_buffer_[0];
                    packet_buffer_.pop_front();
                    packet_cv_.notify_all();
                }

                udp_sender_->send_to(packet->data, packet->length,
                        packet->ip, packet->port);

                if(packet->callback)
                    packet->callback();
                
            }
        });
    }

    void async_sender::stop()
    {
        abort_ = true;
    }

    /**
     * @brief bind to local specific nic card
     * 
     * @param ip 
     * @param port 
     * @return int, if failed return < 0, success return 1
     */
    int async_sender::bind_ip(std::string ip, short port)
    {
        udp_sender_->bind_ip(ip, port);
    }

    /**
     * @brief connect to remote host
     * 
     * @param ip 
     * @param port 
     * @return int 
     */
    int async_sender::connect_host(std::string ip, short port)
    {
        udp_sender_->connect_host(ip, port);
    }

    /**
     * @brief push the data to the buffer, waiting to be send
     * 
     * @param data 
     * @param ip 
     * @param port 
     */
    void async_sender::send_to(uint8_t* data, int length, std::string ip, short port, F&& f)
    {
        int index = -1; // the packet index of the buffer
        index = write();
        while(index == -1)
        {   // loop until get packet buffer
            index = write();
        }

        buffer_[index].data = data;
        buffer_[index].length = length;
        buffer_[index].ip = ip;
        buffer_[index].port = port;
        
        if(f)
            buffer_[index].callback = std::move(f);

        size_++;
    }

    void async_sender::send_to2(uint8_t* data, int length, std::string ip, short port, F&& f)
    {
        std::shared_ptr<data_info> packet = std::make_shared<data_info>();
        packet->data = data;
        packet->length = length;
        packet->ip = ip,
        packet->port = port;
        packet->callback = std::move(f);
                
        std::unique_lock<std::mutex> lock(packet_mutex_);
        if(packet_buffer_.size() < packet_capacity_)
        {
            packet_buffer_.push_back(packet);
            packet_cv_.notify_all();
            return;
        }

        packet_cv_.wait(lock, [this](){return packet_buffer_.size() < packet_capacity_;});
        packet_buffer_.push_back(packet);
        packet_cv_.notify_all();

    }

    int async_sender::write()
    {
        int index = -1;
        if(size_ < (capacity_ - 2))
        {
            index = in_;
            in_++;
            if(in_ == capacity_) in_ = 0;
            //size_++;
        }

        return index;
    }

    int async_sender::read()
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

}} // namespace seeder::net
