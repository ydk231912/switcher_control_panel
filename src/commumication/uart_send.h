#pragma once
// #ifndef UART_SEND_H
// #define UART_SEND_H

#include <string>
#include <vector>
#include <memory>
#include <spdlog/spdlog.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <cerrno>
#include <sstream>
#include <cstring>
#include <atomic>
#include <thread>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <spdlog/common.h>
#include "../switcher.h"
#include "http_client.h"
#include "ascll_convert_16_demo.h"
#include "config.h"

#define LOG(lvl) BOOST_LOG_TRIVIAL(lvl)

namespace seeder {

// Logger
extern std::shared_ptr<spdlog::logger> logger;

/* UART配置结构体 */
struct UARTConfig
{
    std::string port;            // 默认串口
    speed_t baud_rate = B115200; // 默认波特率
};

// UART类，用于处理串口通信
class UART
{
public:
    explicit UART(const UARTConfig &config);
    ~UART();
    void init();
    void start();
    void stop();

    void openPort();
    void closePort();
    void configure();

    UART(const UART &) = delete;            // 禁止拷贝构造函数
    UART &operator=(const UART &) = delete; // 禁止拷贝赋值运算符
    void sendFrame(const std::vector<unsigned char> &data);
    bool checklinkstatus();
    void reconnect();

private:
    int fd_;            // 文件描述符
    std::string port_;  // 串口端口
    speed_t baud_rate_; // 波特率
};

// 数据帧类，用于构造数据帧
class DataFrame
{
public:
    explicit DataFrame(const std::shared_ptr<config::Config> control_panel_config, std::shared_ptr<Switcher> switcher);

    ~DataFrame();

    void init();

    // 构造第一帧数据 -- OLED显示屏数据
    std::vector<unsigned char> construct_frame1(unsigned char frame_header, 
                                                unsigned char device_id,
                                                const std::vector<std::vector<std::vector<std::string>>> &data);
    // 构造第二帧数据 -- 按键灯光数据
    std::vector<unsigned char> construct_frame2(unsigned char frame_header, unsigned char device_id, 
                                                int pgm, int pvw, std::string transition_type, std::vector<int> dsk, int proxy_index, std::vector<int> proxy_sources);
    std::vector<std::vector<std::vector<std::string>>> readHexData(const std::string& filename);
    void writeHexData(const std::string& filename, const std::vector<std::vector<std::vector<std::string>>>& data);

    std::shared_ptr<Switcher> switcher;
    std::shared_ptr<config::Config> control_panel_config;
    
private:
    // 计算数据的校验和
    unsigned char calculate_checksum(const std::vector<unsigned char> &data,
                                     size_t start, size_t end);

    std::vector<int> key_colour = {0,1,2,3,4,5};

    int num_slave = 0;
    int num_screen = 0;
    size_t screen_data_size = 0;
    size_t oled_frame_size = 0;
    int key_each_row_num = 0;
    int switcher_keyboard_id = 0;
    int key_difference_value = 0;
    size_t main_ctrl_keys_size = 0;
    size_t slave_keys_size = 0;
    size_t key_frame_size = 0;
    size_t slave_key_each_row_num = 0;
};

// 主控制类 -- 处理服务器的消息并发送数据帧
class MainController
{
public:
    explicit MainController(const UARTConfig &uart_config1, const UARTConfig &uart_config2, 
                        const std::shared_ptr<config::Config> control_panel_config, 
                        const std::shared_ptr<DataFrame> data_frame, std::shared_ptr<Switcher> switcher);

    ~MainController();

    void init();
    void start();
    void stop();

    void handler_dsk_status(std::vector<bool> dsk_status);
    void handler_nextkey(std::vector<bool> nextkey_status);
    void handler_transition_type(std::string transition_type);
    void handler_status(int pgm_, int pvw_);
    void sendFrames();

    void handle_transition_press(const std::string &transition);
    void handle_proxy_press(const int proxy_idx_, const int proxy_type_);
    void handle_proxy_sources_press(std::vector<int> proxy_sources_);
    void handle_proxy_shift_press(int proxy_source_, int key_idx_);
    void handle_get_key_status(nlohmann::json param);
    
    std::shared_ptr<Switcher> switcher;
    std::shared_ptr<config::Config> control_panel_config;
    std::vector<std::vector<std::vector<std::string>>> screen_frame_hex_data;
    std::shared_ptr<DataFrame> data_frame_;
    std::vector<uint8_t> screen_frame_data;
    std::vector<uint8_t> key_status_data;
    
    std::unique_lock<std::recursive_mutex> acquire_lock() {
        return std::unique_lock<std::recursive_mutex>(mutex);
    }

    UART uart1_;                  // 串口1
    UART uart3_;                  // 串口3
private:
    std::recursive_mutex mutex;
    
    nlohmann::json old_trans_type = nlohmann::json();
    int pgm = 0;
    int pvw = 0;
    std::vector<int> dsk = {1, 1};
    int proxy_type = 0;
    int proxy_index = 0;
    std::vector<int> proxy_sources = {0,0,0,0};
    std::vector<int> last_proxy_sources = {0,0,0,0};
    bool is_proxy_sending = false;

    std::vector<int> nextkey = {0};
    std::string transition_type = "";
    int bkgd = 1;
    int prvtrans = 1;

    int key_each_row_num = 0;
    int max_source_num = 0;
    
};
};// namespace seeder


// #endif // UART_SEND_H
