// #ifndef CAN_RECEIVE_H
// #define CAN_RECEIVE_H
#pragma once

#include <string>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <net/if.h>
#include "../switcher.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <numeric>
#include <termios.h>
#include <vector>

#define LOG(lvl) BOOST_LOG_TRIVIAL(lvl)

namespace seeder{

/* 定义 CAN 接口名称和帧 ID */
#define CAN_INTERFACE "can0"
#define FRAME_ID 0x602 // 主站can消息
#define FRAME1_ID 0x604 // 从站1
#define FRAME2_ID 0x606 // 从站2
#define FRAME3_ID 0x608 // 从站3
#define FRAME4_ID 0x610 // 从站4
#define FRAME5_ID 0x612 // 从站5
#define FRAME6_ID 0x614 // 从站6

// Logger
extern std::shared_ptr<spdlog::logger> logger;

class Switcher;

// 类用于管理 CAN 套接字
class CanSocket
{
public:
    // 构造函数，初始化 CAN 套接字
    explicit CanSocket(const std::string &interface);

    // 析构函数，关闭 CAN 套接字
    ~CanSocket();

    void init();
    void start();
    void stop();

    // 获取套接字文件描述符
    int getSocketFd() const;

private:
    std::string interface_name; // CAN 接口名称
    int socket_fd;              // 套接字文件描述符

    void setupSocket();
    void setNonblocking();

};//class CanSocket

// 按键灯光数据结构体
struct KeyPressState {
  void init(const char *in_name, int in_value) {
    name = in_name;
    value = in_value;
    last_press_time = std::chrono::steady_clock::now();
  }

  bool update(int in_value) {
    if (in_value != value && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_press_time).count() > DEBOUNCE_TIME) {
      LOG(debug) << "key press update: " << name << " old_value=" << value << " new_value=" << in_value;
      value = in_value;
      last_press_time = std::chrono::steady_clock::now();
      return true;
    }
    return false;
  }

  std::chrono::steady_clock::time_point last_press_time; // 上一次按键时间
  int value;
  std::string name;

  static constexpr int DEBOUNCE_TIME = 100;
};

class CanFrameProcessor
{
public:
  CanFrameProcessor(CanSocket &canSocket, std::shared_ptr<seeder::Switcher> sw);
  ~CanFrameProcessor();

  void init();
  void start();
  void stop();

  std::function<void(const std::string &)> on_key_press;

private:
    CanSocket &socket;                    // 引用 CanSocket 实例
    std::thread processing_thread;        // 处理线程
    // std::atomic<bool> stop_flag{false};  // 停止标志
    std::shared_ptr<seeder::Switcher> switcher;
    int dsk = 0;
    int64_t dsk_excut_time = 0;
    int idx = 0;
    // int pgm = 0;
    // int pvw = 0;
    int nextkey = 0;
    int proxy_key_source = 0;

    // 处理 CAN 帧数据
    void processFrames();

    // 处理特定 ID 的 CAN 帧数据
    void processFrame(const can_frame &frame);

    // 处理并发送 CAN 帧数据 -- 推子板
    void handle_FrameData(const can_frame &frame, int frame_id);
    // 处理并发送 CAN 帧数据 -- 从站1
    void handle_FrameData1(const can_frame &frame, int frame_id);
    // 处理并发送 CAN 帧数据 -- 从站2
    void handle_FrameData2(const can_frame &frame, int frame_id);
    // 处理并发送 CAN 帧数据 -- 从站3
    void handle_FrameData3(const can_frame &frame, int frame_id);
    // 处理并发送 CAN 帧数据 -- 从站4
    void handle_FrameData4(const can_frame &frame, int frame_id);
    // 处理并发送 CAN 帧数据 -- 从站5
    void handle_FrameData5(const can_frame &frame, int frame_id);
    // 处理并发送 CAN 帧数据 -- 从站6
    void handle_FrameData6(const can_frame &frame, int frame_id);

    // 计算按键索引的辅助函数
    int calculateidex(unsigned char value, int offset);

    // 打印 CAN 帧数据
    void printCanFrame(const can_frame &frame, const std::string &label);

    // 打印 CAN 帧数据（简化版）
    void printFrameData(const can_frame &frame);

    KeyPressState main_key1_press_state;
    KeyPressState main_key2_press_state;
    KeyPressState main_key3_press_state;
    KeyPressState main_key4_press_state;
    KeyPressState slave1_key1_press_state;
    KeyPressState slave1_key2_press_state;
    KeyPressState slave1_key3_press_state;
    KeyPressState slave2_key1_press_state;
    KeyPressState slave2_key2_press_state;
    KeyPressState slave2_key3_press_state;
    KeyPressState slave3_key1_press_state;
    KeyPressState slave3_key2_press_state;
    KeyPressState slave3_key3_press_state;
    KeyPressState slave4_key1_press_state;
    KeyPressState slave4_key2_press_state;
    KeyPressState slave4_key3_press_state;
    KeyPressState slave5_key1_press_state;
    KeyPressState slave5_key2_press_state;
    KeyPressState slave5_key3_press_state;
    KeyPressState slave6_key1_press_state;
    KeyPressState slave6_key2_press_state;
    KeyPressState slave6_key3_press_state;
    

};//class CanFrameProcessor


};//naemspace seeder
// #endif // CAN_RECEIVE_H