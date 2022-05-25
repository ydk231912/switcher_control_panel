/**
 * udp receiver by linux native socket
 * 
 */
#pragma once

#include <string>
#include <vector>

namespace seeder { namespace net {

class udp_receiver
{
  public:
      udp_receiver();
      
      /**
       * @brief Construct a new udp receiver object
       * 
       * @param mulicast_ip mulicast group ip, e.g. 239.0.0.1
       */
      udp_receiver(std::string mulicast_ip);

      ~udp_receiver();

      /**
       * @brief bind to local specific nic card
       * 
       * @param ip 
       * @param port 
       * @return int, if failed return < 0, success return 1
       */
      int bind_ip(std::string ip, short port);

      /**
       * @brief receive the data in builk via linux socket recvmsg
       * 
       * @param buffer 
       * @param buffer_size 
       * @return int number of bytes receive, -1 for errors
       */
      int receive(uint8_t* buffer, size_t buffer_size);

  private:
      int socket_;
      std::string mulicast_ip_;
};
}} // namespace seeder::net