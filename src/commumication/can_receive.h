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
#include "../switcher.h"

#define LOG(lvl) BOOST_LOG_TRIVIAL(lvl)

namespace seeder{

// const int num_main_keys = 4;
// const int num_slave_keys = 3; 
// const int num_slaves = 6;

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

  void setupSocket();
  void setNonblocking();
  bool checkLinkStatus();
  void reconnect();

  // 获取套接字文件描述符
  int getSocketFd() const;

private:
    std::string interface_name; // CAN 接口名称
    int socket_fd;              // 套接字文件描述符

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

  static constexpr int DEBOUNCE_TIME = 20;
};

class CanFrameProcessor
{
public:
  explicit CanFrameProcessor(CanSocket &canSocket, std::shared_ptr<seeder::Switcher> switcher);
  ~CanFrameProcessor();

  void init();
  void start();
  void stop();

  std::function<void(const std::string &)> on_transition_press;
  std::function<void(void)> on_shift_press;
  std::function<void(void)> on_mode_press;
  std::function<void(const int, const int)> on_proxy_press;

  void handle_panel_status_update(nlohmann::json param);

  std::unique_lock<std::recursive_mutex> acquire_lock() {
      return std::unique_lock<std::recursive_mutex>(mutex);
  }
  std::recursive_mutex mutex;
private:
  CanSocket &socket;
  std::thread processing_thread;
  std::shared_ptr<seeder::Switcher> switcher;
  int idx = 0;
  int switcher_keyboard_id = 0;
  std::vector<int> can_frame_ids;
  int key_each_row_num = 0;
  int key_difference_value = 0;
  int pgm_pvw_shift_amount = 0;
  int proxy_shift_amount = 0;
  // int proxy_mode_status = 0;
  int proxy_type = -1;
  int proxy_type_idx = -1;
  int pgm_pvw_source_sum = 0;
  int proxy_source_sum = 0;
  int key_source_sum = 0;
  int key_source_shift_status = 0;
  int can_array_every = 8;
  int clipper_max = 100;
  size_t clipper_simulate = 4000;
  uint16_t progress_ratio = 101;
  uint16_t progress_ratio_now = 0;
  nlohmann::json is_auto = false;
  bool can_start_clipper = false;
  bool is_up_cut = false;

  // enum Proxy_Type {
  //   KEY,
  //   AUX,
  //   UTL,
  //   MACRO,
  //   SNAPSHOT,
  //   M_e,
  //   UNKNOWN
  // };
  // Proxy_Type proxy_types = KEY;
  // enum Mode_status {
  //   InitValue,
  //   FirstValue,
  //   SecondValue,
  //   ThirdValue,
  //   FourValue,
  //   FiveValue,
  //   LongPress
  // };
  // Mode_status proxy_mode_status = InitValue;

  void processFrames();
  void processFrame(const can_frame &frame);

  void handle_FrameData(const can_frame &frame);
  void handle_FrameData1(const can_frame &frame);
  void handle_FrameData2(const can_frame &frame);
  void handle_FrameData3(const can_frame &frame);
  void handle_FrameData4(const can_frame &frame);
  void handle_FrameData5(const can_frame &frame);
  void handle_FrameData6(const can_frame &frame);

  void handles_pgm_pvw_module(int idx, bool is_pgm);
  void handle_pgm_pvw(int idx, bool is_pgm);
  void update_pgm_pvw_shift_status(int key_each_row_num, bool is_pgm);
  int handle_pgm_pvw_increment(int pgm_pvw_shift_status);
  void handles_proxy_key_module(int idx, bool is_proxy_type);
  void handle_proxy(int idx, bool is_proxy_type);
  int handle_proxy_source_increment(int proxy_source_shift_status);
  void update_proxy_source_shift_status(int key_each_row_num, bool is_proxy_type);

  void update_mode_status();
  void handles_clipper_key(int idx);
  uint16_t clipper_handle(const can_frame &frame);
  uint16_t convertToPercentage(uint8_t highbyte, uint8_t lowbyte);
  int calculateidex(unsigned char value, int offset);

  KeyPressState main_key_press_states[4];
  KeyPressState slave_key_press_states[6][3];
  KeyPressState clipper_state;

  const char* main_Keys[4] = {"main_key1", "main_key2", "main_key3", "main_key4"};
  const char* slave_Keys[6][3] = {
    {"slave1_key1", "slave1_key2", "slave1_key3"},{"slave2_key1", "slave2_key2", "slave2_key3"},
    {"slave3_key1", "slave3_key2", "slave3_key3"},{"slave4_key1", "slave4_key2", "slave4_key3"},
    {"slave5_key1", "slave5_key2", "slave5_key3"},{"slave6_key1", "slave6_key2", "slave6_key3"}
  };
};//class CanFrameProcessor


};//naemspace seeder
// #endif // CAN_RECEIVE_H