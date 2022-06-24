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
#include "util/logger.h"
#include "util/timer.h"


namespace seeder { namespace net {
    udp_sender::udp_sender()
    {
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if(socket_ == -1)
        {
            util::logger->error("Create udp socket client failed!");
            throw std::runtime_error("Create udp socket client failed!");
        }

        // set socketopt, SO_REUSEADDR true, send buffer
        bool bReuseaddr = true;
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(bool));
        int buffer_size = 4000000;
        setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));

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
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.data());
        addr.sin_port = htons(port);

        if(bind(socket_, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
        {
            util::logger->error("bind to ip {}:{} failed!", ip, port);
            return -1;
        }

        return 1;
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
            util::logger->error("connect to ip {}:{} failed!", ip, port);
            return -1;
        }

        return 1;
    }

     /**
       * @brief send the data in builk via linux socket sendto
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

        // util::timer timer;
        auto start_time = util::timer::now();
        auto result = sendto(socket_, data, length, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr));
        auto end_time = util::timer::now();
        packet_count_++;
        if(end_time - start_time > 60000)
        {
            std::cout << "udp sendto time: " << end_time - start_time << "   " << packet_count_ << std::endl;
        }

        // auto send_time = timer.elapsed();
        // while(2100 - send_time > 0)
        // {
        //     send_time = timer.elapsed();
        // }
        //delay_.tv_nsec = delay_.tv_nsec - send_time;
        //nanosleep(&delay_, NULL);
        //std::cout << "udp sendto spend time us: " << send_time << "    " << delay_.tv_nsec << std::endl;
        if (result < 0)
        {
            util::logger->error("send to ip {}:{} udp packet failed!", ip, port);
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

        for(int i = 0; i < size; i++)
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
                util::logger->error("sendmmsg to ip {}:{} udp packet failed!", ip, port);
            }
        //}
        
    }

}} // namespace seeder::net
