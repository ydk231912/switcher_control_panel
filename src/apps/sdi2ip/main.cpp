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

#include <malloc.h>

// test
#include <test.h>
#include "net/udp_receiver.h"

using namespace boost::lockfree;
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
            config.leap_seconds = pt.get<int>("configuration.system.leap-seconds");

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
                channel.format_desc.audio_channels = xml_channel.second.get<int>("audio-channels");
                channel.format_desc.audio_sample_rate = xml_channel.second.get<int>("audio-rate");
                channel.format_desc.audio_samples = xml_channel.second.get<int>("audio-samples");
                channel.rtp_video_type = xml_channel.second.get<int>("rtp-video-type");
                channel.rtp_audio_type = xml_channel.second.get<int>("rtp-audio-type");
                channel.leap_seconds = config.leap_seconds;
                
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
                    //exit(0);
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
    //test();
    //return 0;
    // auto receiver = seeder::net::udp_receiver("239.0.10.21");
    // receiver.bind_ip("192.168.10.103", 20000);

    auto start_time = std::chrono::steady_clock::now();
    int return_code = 0;

    seeder::config config;
    try{
        // parse config file
        config = seeder::parse_config("sdi2ip_config.xml");

        // set logger config
        logger = create_logger(config.console_level, config.file_level, config.log_path, config.max_size, config.max_files);
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return 0;
    }

    try
    {
        auto should_restart = seeder::run(config);
        return_code = should_restart ? 5 : 0; // restart or exit
    }
    catch(std::exception& e)
    {
        logger->error(e.what());
    }
    
    return return_code;
}
