/**
 * @file server.h
 * @author 
 * @brief seeder sdi to st2110 server
 * @version 
 * @date 2022-04-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "server.h"
#include "util/logger.h"

using namespace seeder::util;
namespace seeder
{
    server::server(config& config)
    :config_(config)
    {
    }

    server::~server()
    {
        stop();

        for(auto channel : channel_map_)
        {
            channel.second.reset();
        }
    }

    /**
     * @brief start sdi to st2110 server
     * 
     */
    void server::start()
    {
        //for every sdi channel, create channel, then start handle
        for(auto channel_config : config_.channels)
        {
            try
            {
                auto ch = std::make_shared<channel>(channel_config);
                channel_map_.insert(std::make_pair(channel_config.device_id, ch));
                ch->start();
            }
            catch(const std::exception& e)
            {
                logger->error("Start Channel {} failed. {}", channel_config.device_id, e.what());
                continue;
            }
        }

        // TODO: add controller server

        logger->info("Start sdi to st2110 server completed");
    }

    /**
     * @brief stop sdi to st2110 server
     * 
     */
    void server::stop()
    {
        for(auto channel : channel_map_)
        {
            channel.second->stop();
        }

        logger->info("Stop sdi to st2110 Server completed");
    }

} // namespace seeder