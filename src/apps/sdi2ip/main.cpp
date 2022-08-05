/**
 * @file main.cpp
 * @author 
 * @brief convert SDI data to ST2110, send data by udp
 * @version
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <string>
#include <future>
#include <exception>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "core/util/logger.h"
#include "core/video_format.h"
#include "config.h"
#include "server.h"
#include "core/util/timer.h"

#include <malloc.h>

// for test
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>

#include <net/udp_sender.h>
#include <rtp/packet.h>
#include <functional>
#include <core/frame/frame.h>
#include <memory>
//#include <sdl/output/sdl_output.h>

using namespace seeder::core;
using namespace boost::property_tree;

namespace seeder
{
    void print_info()
    {
        logger->info("############################################################################");
        logger->info("SEEDER SDI to SMPTE ST2110 Server 1.0 ");
        logger->info("############################################################################");
        logger->info("Starting SDI to SMPTE ST2110 Server ");
    }

    // parse config file
    config parse_config(std::string filename)
    {
        config config;
        ptree pt;

        try
        {
            xml_parser::read_xml(filename, pt);

            config.console_level = pt.get<std::string>("configuration.log.console-log-level");
            config.file_level = pt.get<std::string>("configuration.log.file-log-level");
            config.log_path = pt.get<std::string>("configuration.log.log-path");
            config.max_size = pt.get<int>("configuration.log.max-file-size");
            config.max_files = pt.get<int>("configuration.log.max-files");

            auto child = pt.get_child("configuration.channels");
            for(auto xml_channel : child)
            {
                channel_config channel;
                channel.device_id = xml_channel.second.get<int>("device-id");
                channel.video_mode = xml_channel.second.get<std::string>("video-mode");
                channel.multicast_ip = xml_channel.second.get<std::string>("multicast-ip");
                channel.multicast_port = xml_channel.second.get<short>("multicast-port");
                channel.bind_ip = xml_channel.second.get<std::string>("bind-ip");
                channel.bind_port = xml_channel.second.get<short>("bind-port");
                channel.display_screen = xml_channel.second.get<bool>("display-screen");
                channel.format_desc = core::video_format_desc(channel.video_mode);
                config.channels.push_back(channel);
            }
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("Error in read the config.xml file:" + std::string(e.what()));
        }        
        
        return config;
    }

    bool run(config& config)
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future =  promise->get_future();
        auto shutdown = [promise = std::move(promise)](bool restart){
            promise->set_value(restart);
            };

        print_info();

        // start sdi to st2110 server
        std::unique_ptr<server> main_server(new server(config));
        main_server->start();

        // use separate thread for blocking console input
        std::thread([&]() mutable {
            while(true)
            {
                std::string cmd;
                if(!std::getline(std::cin, cmd)) // block waits for the input
                {
                    std::cin.clear();
                    continue;
                }

                if(cmd.empty())
                    continue;
                
                if(boost::iequals(cmd, "EXIT") || boost::iequals(cmd, "Q") || boost::iequals(cmd, "QUIT") || boost::iequals(cmd, "BYE"))
                {
                    logger->info("Received message from Console: {}", cmd);
                    shutdown(false);
                    exit(0);
                    break;
                }
            }
        }).detach();

        // the main thread blocks for waiting to exit or restart
        future.wait();
        main_server.reset();
        return future.get();
    }

} // namespace seeder

int main(int argc, char* argv[])
{
    auto start_time = std::chrono::steady_clock::now();
    int return_code = 0;

    try
    {
        // parse config file
        auto config = seeder::parse_config("sdi2ip_config.xml");

        // set logger config
        logger = create_logger(config.console_level, config.file_level, config.log_path, config.max_size, config.max_files);

        auto should_restart = seeder::run(config);
        return_code = should_restart ? 5 : 0; // restart or exit
    }
    catch(std::exception& e)
    {
        logger->error(e.what());
    }
    
    return return_code;
}


//-------------------------------------------------for test-------------------------------

void frame_test()
{
    std::shared_ptr<frame> frm = std::make_shared<frame>();
    frm->linesize[0] = 100;
    std::cout << frm->linesize[0] <<std::endl;
}

void signal_hander(int signal_no)
{
    std::cout << timer::now() << std::endl;
}

// nanosleep test
void test()
{

    // ///////////////////////////////////////////// setitimer (precision 1us)
    // signal(SIGALRM, signal_hander);
    // struct itimerval timer;
    // timer.it_value.tv_sec = 0;	
    // timer.it_value.tv_usec = 1;
    // timer.it_interval.tv_sec = 0;
    // timer.it_interval.tv_usec = 1;
    // setitimer(ITIMER_REAL, &timer, 0);

    // timespec delay;
    // delay.tv_sec = 0;
    // delay.tv_nsec = 10000;
    struct timeval tv;
    tv.tv_sec = 0;	
    tv.tv_usec = 1000000;
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = 1000;
    while(true)
    {
        /////////////////////////////////////////// select
        timer timer;
        //auto ret = select(0, NULL, NULL, NULL, &tv);
        //auto ret = pselect(0, NULL, NULL, NULL, &req, NULL);
        auto e = timer.elapsed();
        std::cout << e << std::endl;

        ///////////////////////////////////////////nanosleep test
        // timer timer;
        // auto ret = nanosleep(&delay, NULL);
        // auto e = timer.elapsed();
        // std::cout << e << std::endl;

        /////////////////////////////////////////// timer.elapsed test(30ns); std::cout test(1400ns)
        // timer t1;
        // timer t2;
        // auto e2 = t2.elapsed();
        // auto e1 = t1.elapsed();
        // timer t3;
        // std::cout << e1 << "   " << e2 << "  " << e1 - e2 <<std::endl;
        // auto e3 = t3.elapsed();
        // std::cout << e1 << "   " << e3 << std::endl;

        ///////////////////////////////////////////// for test
        // timer t;
        // for(long i = 0; i < 1000; i++) // 300ns
        // {}
        // auto e = t.elapsed();
        // std::cout << e << std::endl;

        // timer t;
        // auto e = t.elapsed();
        // std::cout << e << std::endl;

    }
}

void udp_send_test()
{
    seeder::rtp::packet p;
    seeder::net::udp_sender sender;
    sender.bind_ip("192.168.10.103", 20001);

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(23, &mask);
    if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    {
        std::cout << "bind udp thread to cpu failed" << std::endl;
    }

    while(1){
        sender.send_to(p.get_data_ptr(), p.get_data_size(), "192.168.10.101", 20000);
    }
}

void reference_test(std::shared_ptr<int>& p)
{
    std::cout << "p1 use count: " << p.use_count() << std::endl;
}

std::function<void()> shared_ptr_test()
{
    std::shared_ptr<int> p1(new int(1));
    std::cout << "p1 use count: " << p1.use_count() << std::endl;

    auto f([p1](){ // [&p1]
        std::cout << "f::p1 use count: " << p1.use_count() << std::endl;
    });

    std::cout << "p1 use count: " << p1.use_count() << std::endl;

    return f;

    // auto p2 = p1;
    // std::cout << "p1 use count: " << p1.use_count() << std::endl;
    // std::cout << "p2 use count: " << p2.use_count() << std::endl;

    // p2.reset();
    // std::cout << "p1 use count: " << p1.use_count() << std::endl;
    // std::cout << "p2 use count: " << p2.use_count() << std::endl;

    // p2 = std::make_shared<int>(2);
    // std::cout << "p1 use count: " << p1.use_count() << std::endl;
    // std::cout << "p2 use count: " << p2.use_count() << std::endl;
}