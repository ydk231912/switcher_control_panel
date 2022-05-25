/**
 * udp client by linux native socket
 *
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "udp_sender.h"
#include "util/logger.h"


namespace seeder { namespace net {
    udp_sender::udp_sender()
    {
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if(socket_ == -1)
        {
            util::logger->error("Create udp socket client failed!");
            throw std::runtime_error("Create udp socket client failed!");
        }

        // set SO_REUSEADDR true
        bool bReuseaddr=true;
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(bool));
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

        auto result = sendto(socket_, data, length, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr));
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
    void udp_sender::sendmmsg_to(std::vector<send_data> data, std::string ip, short port)
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

        for(int i = 0; i < data.size(); i++)
        {
            iov[i].iov_base = data.at(i).data;
            iov[i].iov_len = data.at(i).length;
            msg[i].msg_hdr.msg_iov = iov + i;
            msg[i].msg_hdr.msg_iovlen = 1;
            msg[i].msg_hdr.msg_name = (struct sockaddr *) &addr;
            msg[i].msg_hdr.msg_namelen = sizeof(struct sockaddr);
        }

        sendmmsg(socket_, msg, 1, 0);
    }

}} // namespace seeder::net
