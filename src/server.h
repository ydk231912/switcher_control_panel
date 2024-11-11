#pragma once 

#include <chrono>
#include <memory>
#include <thread>

#include "switcher.h"
#include "commumication/can_receive.h"
#include "commumication/uart_send.h"
#include "commumication/http_client.h"
#include "commumication/websocket_client.h"
#include "commumication/config.h"

namespace seeder{

class Server{
public:
    Server(std::string config_path);
    ~Server();

    void start();
    void stop();
    void join();
    
private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder