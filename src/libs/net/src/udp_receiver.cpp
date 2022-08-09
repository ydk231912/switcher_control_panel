/**
 * udp receiver by linux native socket
 *
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "udp_receiver.h"
#include "core/util/logger.h"

using namespace seeder::core;
namespace seeder { namespace net {
    udp_receiver::udp_receiver()
    {
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if(socket_ == -1)
        {
            logger->error("Create udp socket failed!");
            throw std::runtime_error("Create udp socket failed!");
        }
    }

    /**
     * @brief Construct a new udp receiver object
     * 
     * @param mulicast_ip mulicast group ip, e.g. 239.0.0.1
     */
    udp_receiver::udp_receiver(std::string multicast_ip)
    :mulicast_ip_(multicast_ip)
    {
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if(socket_ == -1)
        {
            logger->error("Create udp socket client failed!");
            throw std::runtime_error("Create udp socket client failed!");
        }

        int buffer_size = 4000000;
        setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

        // join multicast group
        struct ip_mreq multi_adr;
        multi_adr.imr_multiaddr.s_addr = inet_addr(multicast_ip.data());
        multi_adr.imr_interface.s_addr = htonl(INADDR_ANY);
        auto ret = setsockopt(socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&multi_adr, sizeof(multi_adr));
        if(ret == -1)
        {
            logger->error("Join udp multicast group {} failed!", mulicast_ip_);
            throw std::runtime_error("Join udp multicast group failed!");
        }
    }

    udp_receiver::~udp_receiver()
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
    int udp_receiver::bind_ip(std::string ip, short port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.data());
        addr.sin_port = htons(port);

        if(bind(socket_, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
        {
            logger->error("bind to ip {}:{} failed!", ip, port);
            return -1;
        }

        return 1;
    }

    /**
     * @brief receive the data in builk via linux socket recvmsg
     * 
     * @param buffer 
     * @param buffer_size 
     * @return int number of bytes receive, -1 for errors
     */
    int udp_receiver::receive(uint8_t* buffer, size_t buffer_size)
    {
        auto len = recvfrom(socket_, buffer, buffer_size, 0, NULL, 0);
        return len;
    }

}} // namespace seeder::net
