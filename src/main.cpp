/**
 * @file main.cpp
 * @author 
 * @brief for test
 * @version 
 * @date 2022-05-17
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>
#include <string>

#include <boost/thread/thread.hpp>

#include "rtp_st2110.h"
#include "net/udp_client.h"
#include "video_play.h"
#include "sdl_consumer.h"
#include "core/util/logger.h"


// test udp multicast
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace seeder;
using namespace seeder::core;

std::string filename ="/home/seeder/d.MXF";
//std::string filename ="/home/seeder/test.mp4";
//std::string multicast_address = "192.168.123.254";
std::string multicast_address = "239.0.10.1";
short port = 20000;
rtp_st2110 rtp_;
video_play play_;
//net::udp_client udp_client_;
sdl_consumer sdl_;

// decode vedio file, generate frame
void ffmpeg_producer()
{
    while(true) // loop play
    {
        play_.play(filename);
    }
}

// generate rtp packet
void rtp_producer()
{
    while(true)
    {
        auto frame = rtp_.receive_frame();
        if(frame.video)
        {
            rtp_.rtp_handler(frame);
            //auto p = frame.get();
            //av_frame_free(&p);
        }
        else
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
        }
    }
}

// send rtp packet by udp
void udp_consumer()
{
    net::udp_client udp_client_;
    while(true)
    {
        auto packet = rtp_.receive_packet();
        if(packet)
        {
            udp_client_.send_to(packet->get_data_ptr(), packet->get_data_size(), multicast_address, port);
        }
        else
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
        }
    }
}

// display frame
void sdl_display()
{
    while(true)
    {
        auto frame = sdl_.receive_frame();
        if(frame)
        {
            sdl_.handler(frame);
        }
        else
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        }
    }
}

// play video
void run()
{
    while(true)
    {
        auto frame = play_.receive_frame();
        
        if(frame.video)
        {
            // push to rtp
            rtp_.push_frame(frame);

            // push to sdl
            sdl_.push_frame(frame.video);
        }

        boost::this_thread::sleep_for(boost::chrono::milliseconds(40));  // 25 frames per second
    }
}


int test_net_speed()
{
    net::udp_client udp_client_;
    auto packet = std::make_shared<rtp::packet>();
    while(true)
    {
        udp_client_.send_to(packet->get_data_ptr(), packet->get_data_size(), multicast_address, port);
    }

    return 0;
}

int test_multicast()
{
    int sd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    ssize_t res;
    unsigned int addrlen;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5005);
    addr.sin_addr.s_addr = inet_addr("239.0.10.2");
    addrlen = sizeof(addr);
    char * message = "udp multicast message";

    while(true)
    {
        res = sendto(sd, message, sizeof(message), 0, (struct sockaddr *)&addr, addrlen);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10)); 
    }

}

int main()
{
    logger->info("start server!");

    boost::thread play_thread(&run);
    boost::thread ffmpeg_thread(&ffmpeg_producer);
    boost::thread rtp_thread(&rtp_producer);
    //boost::thread udp_thread(&udp_consumer);
    boost::thread_group udp_group;
    for(int i =0; i < 20; i++)
    {
        udp_group.create_thread(&udp_consumer);
    }
    boost::thread sdl_thread(&sdl_display);
    play_thread.join();
    ffmpeg_thread.join();
    rtp_thread.join();
    sdl_thread.join();
    udp_group.join_all();

    return 0;
}







// ffmpeg lib test
//int ffmpeg_test(std::string filename)
// {
//     AVFormatContext *fmt_ctx = NULL;
//     int ret = 0;
//     if((ret = avformat_open_input(&fmt_ctx, filename.data(), 0, 0)) < 0)
//     {
//         printf( "Could not open input file.");
//         return 0;
//     }

//     if((ret = avformat_find_stream_info(fmt_ctx, 0)) < 0)
//     {
//         printf( "Failed to retrieve input stream information");
//         avformat_close_input(&fmt_ctx);
//         return 0;
//     }

//     printf("3ifmt_ctx->video_codec_id:%d\n",fmt_ctx->streams[0]->codecpar->codec_id);

//     if(fmt_ctx->streams[0]->codecpar->codec_id==AV_CODEC_ID_H264)
//     {
//         printf("is h264 file\n");
//         avformat_close_input(&fmt_ctx);
//         return 1;
//     }
//     else
//     {
//         printf("is not h264 file\n");
//         avformat_close_input(&fmt_ctx);
//         return 0;
//     }

//     return 0;

// }

