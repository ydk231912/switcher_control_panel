/**
 * @file test.h
 * @author 
 * @brief performance test
 * @version 0.1
 * @date 2022-08-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

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
#include <boost/lockfree/spsc_queue.hpp>
#include "rtp/packet.h"
#include "core/executor/executor.h"

#include <atomic>

using namespace boost::lockfree;
using namespace seeder::core;
using namespace boost::property_tree;



/**
 * @brief 
 * 
 */
void logger_test()
{
    
}
void test(){
    logger_test();
}

/**
 * @brief pointer test
 * uint8_t*, uint32_t* test
 * bmdFormat10BitYUV : ‘v210’ 4:2:2 Representation test
 */
void pointer_test()
{
    uint32_t* p2 = (uint32_t*)malloc(16);

    // bmdFormat10BitYUV
    // 0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // * * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //1073741823  3fffffff
    //4294967295  ffffffff

    p2[0] = 0x3fffffff;
    p2[1] = 0x3fffffff;
    p2[2] = 0x3fffffff;
    p2[3] = 0x3fffffff;

    uint32_t* p3 = (uint32_t*)malloc(16);
    timer t1;
    // 100000loop spend 550us
    for(int i = 0; i < 100000; i++)
    {
        p3[0] = p2[0] | p2[1] << 30;
        p3[1] = p2[1] | p2[2] << 30;
        p3[2] = p2[2] | p2[3] << 30;
        p3[3] = p2[3];
    }
    std::cout << "loop 100000 spend time: " << t1.elapsed() << std::endl;

    for(int i = 0; i < 4; i++)
    {
        std::cout << p3[i] << std::endl;
    }

    auto p1 = (uint8_t*)p3;
    for(int i = 0; i < 16; i++)
    {
        std::cout << (int)p1[i] << std::endl;
    }
    
}
// void test(){
//     pointer_test();
// }

/**
 * @brief thread speed test
 * loop 10000 async call spend time: 8.85016e+08
 * 1 thread call spend time: 65us
 */
void thread_test()
{
    using namespace seeder::core;

    atomic<long> n;
    n = 0;
    timer t1;
    for(long i = 0; i < 1000000; i++)
    {
        n++;
    }
    std::cout << "loop 1000000 spend time: " << t1.elapsed() << std::endl;

    //thread
    n = 0;
    auto f = [&](){
        //n++;
    };
    
    // loop 10000 async call spend time: 8.85016e+08
    timer t2;
    for(long i = 0; i < 10000; i++)
    {
        auto fut = std::async(std::launch::async, f);
    }
    //while(n < 1000000){}
    std::cout << "loop 1000 thread call spend time: " << t2.elapsed() << std::endl;

    // 1 thread call spend time: 65us
    timer t3;
    //for(long i = 0; i < 10000; i++)
    {
        auto th = std::thread(f);
        th.join();
    }
    //while(n < 1000000){}
    std::cout << "loop 1000 thread call spend time: " << t3.elapsed() << std::endl;
}

// void test(){
//     thread_test();
// }

/**
 * @brief packet allocate and free test
 * 2.622 ms: allocate and free 15376 packet
 * 1.8ms: memcpy 15376 packet
 * 14.26ms: deque.push and pop 15376 packet the two thread
 * 20.05ms: spsc_queue push and pop 15376 packet the two thread
 * 0.3ms: atomic int ++/-- in two thread
 * 0.27s: 1000000000 simple loop, 3 times in 1 ns
 */
void pakcet_test()
{
    // cpu speed test
    seeder::core::timer timer1;
    for(long i = 0; i < 1000000000; i++)
    {
    }
    std::cout << "loop 1000000000:" << timer1.elapsed() << std::endl;
    return;

    /////////////////////// atomic speed test
    std::atomic<int> size;
    seeder::core::timer t0;
    auto t1 = std::thread([&](){
        for(int i = 0; i < 15376; i++)
        {
            size++;
        }
    });
    auto t2 = std::thread([&](){
        for(int i = 0; i < 15376; i++){
            size--;
        }
    });
    t1.join();
    t2.join();
    std::cout << "15376 atomit int ++ :" << t0.elapsed() << std::endl;
    return;



    // shared_ptr test
    // seeder::core::timer t;
    // for(int i = 0; i < 15376; i++)
    // {
    //     std::shared_ptr<seeder::rtp::packet> p = std::make_shared<seeder::rtp::packet>();
    // }
    // std::cout << t.elapsed() << std::endl;

    // memory copy test
    seeder::core::timer t;
    for(int i = 0; i < 15376; i++)
    {
        seeder::rtp::packet p1;
        seeder::rtp::packet p2;
        memcpy(p2.get_data_ptr(), p1.get_data_ptr(), p1.get_size());
    }
    std::cout << t.elapsed() << std::endl;
    return;

    // multi thread lock test
    // std::mutex packet_mutex_;
    // std::deque<std::shared_ptr<seeder::rtp::packet>> packet_buffer_;
    // std::size_t packet_capacity_ = 1000;
    // std::condition_variable packet_cv_;
    // seeder::core::timer t;
    // auto t1 = std::thread([&](){
    //     for(int i = 0; i < 15376; i++)
    //     {
    //         std::unique_lock<std::mutex> lock(packet_mutex_);
    //         packet_cv_.wait(lock, [&](){return packet_buffer_.size() < packet_capacity_;});
    //         std::shared_ptr<seeder::rtp::packet> p = std::make_shared<seeder::rtp::packet>();
    //         packet_buffer_.push_back(p);
    //         packet_cv_.notify_all();
    //     }
    // });
    // auto t2 = std::thread([&](){
    //     for(int i = 0; i < 15376; i++){
    //         std::unique_lock<std::mutex> lock(packet_mutex_);
    //         packet_cv_.wait(lock, [&](){return !packet_buffer_.empty();});
    //         auto p = packet_buffer_[0];
    //         packet_buffer_.pop_front();
    //         packet_cv_.notify_all();
    //     }
    // });

    // boost spsc_queue
    // spsc_queue<std::shared_ptr<seeder::rtp::packet>, capacity<15376>> b;
    // seeder::core::timer t;
    // auto t = std::thread([&](){
    //     for(int i = 0; i < 15376; i++)
    //     {
    //         std::shared_ptr<seeder::rtp::packet> p = std::make_shared<seeder::rtp::packet>();
    //         b.push(p);
    //     }
    // });
    // auto t4 = std::thread([&](){
    //     for(int i = 0; i < 15376; i++){
    //         std::shared_ptr<seeder::rtp::packet> p;
    //         b.pop(p);
    //     }
    // });

    // t1.join();
    // t2.join();
    //std::cout << t.elapsed() << std::endl;
}

//spsc_queue.push 15376 packet need 1 ms
//std::deque.push 15376 packet need 0.514 ms
void queue_test()
{
    std::shared_ptr<seeder::rtp::packet> p = std::make_shared<seeder::rtp::packet>();
    spsc_queue<std::shared_ptr<seeder::rtp::packet>, capacity<15376>> packet_bf_;
    std::deque<std::shared_ptr<seeder::rtp::packet>> bf2;
    int i = 0;
    seeder::core::timer t;
    while(i < 15376){   //need 2.27 ms
        //packet_bf_.push(p); 
        //packet_bf_.pop(p);
        bf2.push_back(p);
        i++;
    }
    std::cout << t.elapsed() << std::endl;
}


/**
 * @brief udp send speed test
 * 700MB/s one thread/socket
 * 26ms: send 15376 packet on one socket
 * 68ms: send 15376 packet by executor asynchronous
 */
void udp_send_test()
{
    std::shared_ptr<seeder::rtp::packet> p = std::make_shared<seeder::rtp::packet>();
    std::shared_ptr<seeder::net::udp_sender> sender1 = std::make_shared<seeder::net::udp_sender>();
    std::shared_ptr<seeder::net::udp_sender> sender2 = std::make_shared<seeder::net::udp_sender>();
    std::shared_ptr<seeder::net::udp_sender> sender3 = std::make_shared<seeder::net::udp_sender>();
    std::shared_ptr<seeder::net::udp_sender> sender4 = std::make_shared<seeder::net::udp_sender>();
    std::shared_ptr<seeder::net::udp_sender> sender5 = std::make_shared<seeder::net::udp_sender>();

    // send 15376 packet speed test: 30ms
    seeder::core::timer t;
    for(int i = 0; i < 15376; i++)
    {
       sender1->send_to(p->get_data_ptr(), p->get_data_size(), "239.0.20.28", 20000);
    }
    std::cout << "send 15376 packet: " << t.elapsed() << std::endl;

    // send 15376 packet in executor: 68ms
    executor executor_;
    seeder::core::timer timer2;
    for(int i = 0; i < 15376; i++)
    {
        auto f = [&]() mutable { sender1->send_to(p->get_data_ptr(), p->get_data_size(), "239.0.20.28", 20000);};
        executor_.execute(std::move(f));
    }
    std::cout << "send 15376 packet by executor: " << timer2.elapsed() << std::endl;
    return;

    // cpu_set_t mask;
    // CPU_ZERO(&mask);
    // CPU_SET(23, &mask);
    // if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    // {
    //     std::cout << "bind udp thread to cpu failed" << std::endl;
    // }

    auto t1 = std::thread([sender1, p](){
                  while(1){
                      sender1->send_to(p->get_data_ptr(), p->get_data_size(), "239.0.20.28", 20000);
                  }
              });

    auto t2 = std::thread([sender1, p](){
                  while(1){
                      sender1->send_to(p->get_data_ptr(), p->get_data_size(), "239.0.20.28", 20000);
                  }
              });

    // auto t3 = std::thread([sender1, p](){
    //               while(1){
    //                   sender1->send_to(p->get_data_ptr(), p->get_data_size(), "239.0.20.28", 20000);
    //               }
    //           });

    // auto t4 = std::thread([sender1, p](){
    //               while(1){
    //                   sender1->send_to(p->get_data_ptr(), p->get_data_size(), "239.0.20.28", 20000);
    //               }
    //           });

    // auto t5 = std::thread([sender1, p](){
    //               while(1){
    //                   sender1->send_to(p->get_data_ptr(), p->get_data_size(), "239.0.20.28", 20000);
    //               }
    //           });
    
    t1.join();
    t2.join();
    // t3.join();
    // t4.join();
    // t5.join();
}

// avframe test
void frame_test()
{
    std::shared_ptr<frame> frm = std::make_shared<frame>();
    frm->video->linesize[0] = 100;
    std::cout << frm->video->linesize[0] <<std::endl;
}


void signal_hander(int signal_no)
{
    std::cout << timer::now() << std::endl;
}


// nanosleep test
void sleep_test()
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


