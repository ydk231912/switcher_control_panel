/**
 * udp client by linux native socket
 *
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>


#include "udp_sender.h"
#include "core/util/logger.h"
#include "core/util/timer.h"

using namespace seeder::core;
namespace bai = boost::asio::ip;
namespace seeder { namespace net {
    
    // struct impl
    // {
    //     boost::asio::io_service io_service_;
    //     bai::udp::socket socket_;

    //     impl() : io_service_(), socket_(io_service_, bai::udp::endpoint(boost::asio::ip::udp::v4(), 0)) {}
    // };

    udp_sender::udp_sender()
    : io_service_(), boost_socket_(io_service_, bai::udp::endpoint(bai::udp::v4(), 0))
    {
        // boost::asio::ip::udp::endpoint endpoint_1(boost::asio::ip::udp::v4(),0);
        // boost::asio::ip::address addr = boost::asio::ip::address::from_string("192.168.10.103");
        // endpoint_1.address(addr);
        // boost_socket_ = boost::asio::ip::udp::socket(io_service_, endpoint_1);

        //socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        socket_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(socket_ == -1)
        {
            logger->error("Create udp socket client failed!");
            throw std::runtime_error("Create udp socket client failed!");
        }

        // set socketopt, SO_REUSEADDR true, send buffer
        bool bReuseaddr = true;
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(bool));
        int buffer_size = 60000000;
        auto ret = setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
        setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

        // int32_t get_size;
        // socklen_t len = sizeof(int);
        // getsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &get_size, &len);

        // ret = fcntl(socket_, F_SETFL, SOCK_NONBLOCK);

        // int loop = 1;
        // ret = setsockopt(socket_,IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

        // struct timeval timeout;
        // timeout.tv_sec = 0;
        // timeout.tv_usec = 60;
        // ret = setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

        delay_.tv_sec = 0;
        delay_.tv_nsec = 4725; // 2us

    }

    udp_sender::~udp_sender()
    {
        close(socket_);
    }

    /**
       * @brief bind to local specific nic card
       * 
       * @param ip 
       * @param port 
       * @return int, if failed return < 0, success return 1
       */
    int udp_sender::bind_ip(std::string ip, short port)
    {
        auto addr = inet_addr(ip.data());
        auto ret = setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_IF, (char*)&addr, sizeof(addr));

        struct sockaddr_in ad;
        memset(&addr, 0, sizeof(addr));
        ad.sin_family = AF_INET;
        ad.sin_addr.s_addr = inet_addr(ip.data());
        ad.sin_port = htons(port);

        if(bind(socket_, (struct sockaddr *)&ad, sizeof(struct sockaddr)) < 0)
        {
            logger->error("bind to ip {}:{} failed!", ip, port);
            return -1;
        }

        // return 1;


        // bai::udp::resolver resolver(io_service_);
        // bai::udp::resolver::query query(bai::udp::v4(), ip, std::to_string(port));
        // auto endpoint_iter = resolver.resolve(query);
        // auto end = endpoint_iter.begin();


        // boost::asio::ip::udp::endpoint endpoint_1(boost::asio::ip::udp::v4(),20002);
        // boost::asio::ip::address addr = boost::asio::ip::address::from_string("192.168.10.103");
        // endpoint_1.address(addr);
        // boost_socket_.bind(endpoint_1);
    }

    /**
     * @brief connect to remote host
     * 
     * @param ip 
     * @param port 
     * @return int 
     */
    int udp_sender::connect_host(std::string ip, short port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.data());
        addr.sin_port = htons(port);

        if(connect(socket_, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
        {
            logger->error("connect to ip {}:{} failed!", ip, port);
            return -1;
        }

        return 1;
    }

     /**
       * @brief send the data via linux socket sendto
       * 
       * @param data 
       * @param ip 
       * @param port 
       */
    void udp_sender::send_to(uint8_t* data, int length, std::string ip, short port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.data());
        addr.sin_port = htons(port);

        auto start_time = timer::now();
        auto result = sendto(socket_, data, length, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr));
        if (result < 0)
        {
            logger->error("send to ip {}:{} udp packet failed!", ip, port);
        }

        // packet_count_++;
        // auto end_time = timer::now();
        // if(end_time - start_time > 60000)
        // {
        //     std::cout << "udp sendto time:  " << end_time - start_time << " , packet: " << packet_count_ - last_packet << std::endl;
        //     last_packet = packet_count_;
        // }
    }

    void udp_sender::async_send_to(uint8_t* data, int length, std::string ip, short port)
    {
        bai::udp::resolver resolver(io_service_);
        bai::udp::resolver::query query(bai::udp::v4(), ip, std::to_string(port));
        auto endpoint_iter = resolver.resolve(query);

        auto start_time = timer::now();
        boost_socket_.send_to(boost::asio::buffer(data, length), *endpoint_iter);
        auto end_time = timer::now();
        packet_count_++;
        if(end_time - start_time > 60000)
        {
            std::cout << "udp sendto time:  " << end_time - start_time << "  packet: " << packet_count_ - last_packet << std::endl;
            last_packet = packet_count_ ;  
        }

    }

     /**
       * @brief send the data in builk via linux socket sendmmsg
       * 
       * @param data 
       * @param ip 
       * @param port 
       */
    void udp_sender::sendmmsg_to(std::vector<send_data> data, std::string& ip, short port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.data());
        addr.sin_port = htons(port);

        auto size = data.size();
        struct mmsghdr msg[size];
        struct iovec iov[size];
        memset(msg, 0 , sizeof(msg));
        memset(iov, 0, sizeof(iov));

        for(auto i = 0; i < size; i++)
        {
            iov[i].iov_base = data.at(i).data;
            iov[i].iov_len = data.at(i).length;
            msg[i].msg_hdr.msg_iov = iov + i;
            msg[i].msg_hdr.msg_iovlen = 1;
            msg[i].msg_hdr.msg_name = (struct sockaddr *) &addr;
            msg[i].msg_hdr.msg_namelen = sizeof(struct sockaddr);
        }

        //while(true){
            auto result = sendmmsg(socket_, msg, size, 0);
            if (result < 0)
            {
                logger->error("sendmmsg to ip {}:{} udp packet failed!", ip, port);
            }
        //}
        
    }

}} // namespace seeder::net
