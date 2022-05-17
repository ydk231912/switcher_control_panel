/**
 * udp client by boost asio
 *
 */

#include "udp_client2.h"

using namespace boost::asio;

namespace seeder { namespace net {
    udp_client::udp_client()
    : io_service_()
    , socket_(io_service_, ip::udp::endpoint(ip::udp::v4(), 0))
    {
        socket_.set_option(ip::udp::socket::reuse_address(true));
        ip::udp::endpoint ep(ip::address::from_string("192.168.123.40"), 20001);
        //ip::udp::endpoint ep(ip::address::from_string("0.0.0.0"), 8878);
        //ip::udp::endpoint ep(ip::address_v4::any(), 8878);
        boost::system::error_code ec;
        socket_.bind(ep, ec);
    }

    udp_client::udp_client(std::string ip, std::string port)
        : io_service_()
        , socket_(io_service_, ip::udp::endpoint(ip::udp::v4(), 0))
    {
        try
        {
            ip::udp::resolver        resolver(io_service_);
            ip::udp::resolver::query query(ip::udp::v4(), ip, port);
            auto                     endpoint_iter = resolver.resolve(query);
            endpoint_                              = *endpoint_iter;
            socket_.connect(endpoint_);
        }
        catch(...)
        {}
    }

    udp_client::~udp_client()
    {
        socket_.close();
    }

    void udp_client::send_to(uint8_t* data, int length)
    {
        try
        {
            socket_.send_to(buffer(data, length), endpoint_);
        }
        catch(...)
        {
            
        }
        
        //socket_.send(buffer(data, length));
        /*socket_.async_send_to(buffer(data, length),
                              endpoint_,
                              [](const boost::system::error_code& error, std::size_t bytes_transferred) {});*/
    }

    void udp_client::send_to(uint8_t* data, int length, std::string ip, short port)
    {
        try
        {
            // ip::udp::resolver        resolver(io_service_);
            // ip::udp::resolver::query query(ip::udp::v4(), ip, port);
            // auto endpoint_iter = resolver.resolve(query);
            ip::udp::endpoint ep(ip::address::from_string(ip), port);

            socket_.send_to(buffer(data, length), ep);
        }
        catch(...)
        {
            int i = 1;
        }
    }


}} // namespace seeder::net
