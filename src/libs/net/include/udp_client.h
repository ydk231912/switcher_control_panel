/**
 * udp client by boost asio
 *
 */
#pragma once

#include <string>
#include <boost/asio.hpp>

namespace seeder { namespace net {

class udp_client
{
  public:
      udp_client();
      udp_client(std::string ip, std::string port);
      ~udp_client();

      void send_to(uint8_t* data, int length, std::string ip, short port);
      void send_to(uint8_t* data, int length);

      

  private:
      boost::asio::io_service io_service_;
      boost::asio::ip::udp::socket socket_;
      boost::asio::ip::udp::endpoint endpoint_;
};
}} // namespace seeder::net