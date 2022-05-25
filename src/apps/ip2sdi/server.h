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