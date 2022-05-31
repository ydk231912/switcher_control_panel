/**
 * @file server.h
 * @author 
 * @brief seeder st2110 to sdi server
 * @version 
 * @date 2022-05-18
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
     * @brief start st2110 to sdi server
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

        logger->info("Start st2110 to sdi server completed");
    }

    /**
     * @brief stop st2110 to sdi server
     * 
     */
    void server::stop()
    {
        for(auto channel : channel_map_)
        {
            channel.second->stop();
        }

        logger->info("Stop st2110 to sdi Server completed");
    }

} // namespace seeder