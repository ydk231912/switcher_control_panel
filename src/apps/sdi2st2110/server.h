/**
 * @file server.h
 * @author your name (you@domain.com)
 * @brief seeder sdi to st2110 server
 * @version 0.1
 * @date 2022-04-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <map>

#include "config.h"
#include "channel.h"

namespace seeder
{
    class server
    {
      public:
        server(config&);
        ~server();

        void start();

        void stop();

      private:
        config config_;
        std::map<int, std::shared_ptr<channel>> channel_map_;
    };
}