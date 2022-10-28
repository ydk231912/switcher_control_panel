/**
 * udp client send data asynchronously
 *
 */
#pragma once

#include <memory>
#include <thread>
#include <string>
#include <vector>
#include <atomic>
#include <functional>
#include <mutex>
#include <deque>
#include <condition_variable>

#include <net/udp_sender.h>

namespace seeder { namespace net {

struct data_info
{
    uint8_t* data;
    int length;
    std::string ip;
    short port;
    std::function<void()> callback;
};

class async_sender
{
  public:
      using F = std::function<void()>;
      async_sender(uint32_t capacity);
      async_sender();
      ~async_sender();

      void init();

      /**
       * @brief bind to local specific nic card
       * 
       * @param ip 
       * @param port 
       * @return int, if failed return < 0, success return 1
       */
      int bind_ip(std::string ip, short port);

      /**
       * @brief connect to remote host
       * 
       * @param ip 
       * @param port 
       * @return int 
       */
      int connect_host(std::string ip, short port);

      void start();
      void start2();

      void stop();

      /**
       * @brief send the data in builk via linux socket sendto
       * 
       * @param data 
       * @param ip 
       * @param port 
       */
      void send_to(uint8_t* data, int length, std::string ip, short port, F&& f);
      void send_to2(uint8_t* data, int length, std::string ip, short port, F&& f);

  private:
    int write();

    int read();

  private:
      bool abort_ = false;
      std::shared_ptr<net::udp_sender> udp_sender_;

      // thread
      std::thread thread_;

      // send data buffer
      std::vector<data_info> buffer_;
      uint32_t capacity_; // packets number
      std::atomic<uint32_t> size_;  // current how many packets in the buffer
      std::atomic<uint32_t> in_;  // write packet position
      std::atomic<uint32_t> out_; // read packet position

      std::deque<std::shared_ptr<data_info>> packet_buffer_;
      std::size_t packet_capacity_ = 11544;
      std::mutex packet_mutex_;      
      std::condition_variable packet_cv_;

};
}} // namespace seeder::net