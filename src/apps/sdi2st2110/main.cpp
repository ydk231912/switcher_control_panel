/**
 * @file main.cpp
 * @author your name (you@domain.com)
 * @brief convert SDI data to ST2110, send data by udp
 * @version 0.1
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "util/logger.h"

using namespace seeder::util;

namespace seeder
{
    void print_info()
    {
        logger->info("############################################################################");
        logger->info("SEEDER SDI to SMPTE ST2110 Server 1.00");
        logger->info("############################################################################");
        logger->info("Starting SDI to SMPTE ST2110 Server ");
    }

    void run()
    {
        print_info();

        //start server

    }

} // namespace seeder


int main(int argc, char* argv[])
{
    seeder::run();

    return 0;
}