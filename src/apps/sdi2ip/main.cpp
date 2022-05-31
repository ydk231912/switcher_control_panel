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

#include "util/logger.h"
#include "core/video_format.h"
#include "config.h"
#include "server.h"

#include <malloc.h>

using namespace seeder::util;
using namespace boost::property_tree;

namespace seeder
{
    void print_info()
    {
        logger->info("############################################################################");
        logger->info("SEEDER SDI to SMPTE ST2110 Server 1.00");
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
        auto shutdown = [promise = std::move(promise)](bool restart){promise->set_value(restart);};

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
                    util::logger->info("Received message from Console: {}", cmd);
                    shutdown(false);
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