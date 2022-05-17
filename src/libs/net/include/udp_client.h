/**
 * udp client by linux native socket
 *
 */
#pragma once

#include <string>
#include <vector>

namespace seeder { namespace net {

struct send_data
{
    uint8_t* data;
    int length;
};

class udp_client
{
  public:
      udp_client();
      ~udp_client();

      /**
       * @brief bind to local specific nic card
       * 
       * @param ip 
       * @param port 
       * @return int, if failed return < 0, success return 1
       */
      int bind_ip(std::string ip, short port);

      /**
       * @brief send the data in builk via linux socket sendto
       * 
       * @param data 
       * @param ip 
       * @param port 
       */
      void send_to(uint8_t* data, int length, std::string ip, short port);

      /**
       * @brief send the data in builk via linux socket sendmmsg
       * 
       * @param data 
       * @param ip 
       * @param port 
       */
      void sendmmsg_to(std::vector<send_data> data, std::string ip, short port);

  private:
      int socket_;
};
}} // namespace seeder::net