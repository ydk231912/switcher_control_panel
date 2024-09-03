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
#include "http_client.h"
#include <iomanip>
#include <spdlog/common.h>

namespace seeder {

// 定义数据帧大小
constexpr size_t SCREEN_DATA_SIZE = 40; // 每个屏幕数据大小
constexpr int NUM_SCREENS = 3;          // 屏幕数量
constexpr int NUM_SLAVES = 2;           // 从机数量
constexpr size_t FRAME1_SIZE = 1 + 1 + NUM_SCREENS * SCREEN_DATA_SIZE * NUM_SLAVES + 1 + 1; // 第一帧数据大小

constexpr size_t MAIN_CTRL_KEYS_SIZE = 25;                                                          // 主控板按键灯光数据大小
constexpr size_t SLAVE_KEYS_SIZE = 24;                                                              // 从机按键灯光数据大小
constexpr int NUM_SLAVE_KEYS = 2;                                                                   // 从机数量
constexpr size_t FRAME2_SIZE = 1 + 1 + MAIN_CTRL_KEYS_SIZE + NUM_SLAVE_KEYS * SLAVE_KEYS_SIZE  + 1; // 第二帧数据大小

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

    UART(const UART &) = delete;            // 禁止拷贝构造函数
    UART &operator=(const UART &) = delete; // 禁止拷贝赋值运算符

    // 发送数据帧
    void sendFrame(const std::vector<unsigned char> &data);

private:
    int fd_;            // 文件描述符
    std::string port_;  // 串口端口
    speed_t baud_rate_; // 波特率

    // 打开串口
    void openPort();
    // 关闭串口
    void closePort();
    // 配置串口参数
    void configure();

};

// 数据帧类，用于构造数据帧
class DataFrame
{
public:
    // 构造第一帧数据 -- OLED显示屏数据
    std::vector<unsigned char> construct_frame1(unsigned char frame_header,
                                                       unsigned char device_id);

    // 构造第二帧数据 -- 按键灯光数据
    std::vector<unsigned char> construct_frame2(unsigned char frame_header,
                                                unsigned char device_id, int pgm,
                                                int pvw, std::string transition_type, std::vector<int> dsk);
    std::vector<std::vector<std::vector<std::string>>> readHexData(const std::string& filename);
    void writeHexData(const std::string& filename, const std::vector<std::vector<std::vector<std::string>>>& data);
    std::vector<std::vector<std::vector<std::string>>> file_event_handlers();
    
private:
    // 计算数据的校验和
    unsigned char calculate_checksum(const std::vector<unsigned char> &data,
                                     size_t start, size_t end);

    
};

// 主控制类 -- 处理服务器的消息并发送数据帧
class MainController
{
public:
    MainController(const UARTConfig &uart_config1, const UARTConfig &uart_config3);

    // 接收服务器的消息，构造并发送数据帧
    void handler_dsk_status(std::vector<bool> dsk_status);
    void handler_nextkey(std::vector<bool> nextkey_status);
    void handler_mode(std::vector<bool> mode);
    void handler_bkgd(std::vector<bool> bkgd);
    void handler_prevtrans(std::vector<bool> prvtrans);
    void handler_shift(std::vector<bool> shift);
    void handler_transition_type(std::string transition_type);
    void handler_status(int pgm_, int pvw_);
    // 停止接收
    void stop();
    // 发送数据帧
    void sendFrames();

private:
    UART uart1_;                  // 串口1
    UART uart3_;                  // 串口3
    DataFrame data_frame_;        // 数据帧对象
    std::thread sender_thread_;   // 发送线程
    std::thread printer_thread_;  // 打印线程
    std::atomic<bool> stop_flag_; // 停止标志
    int pgm = 1;
    int pvw = 1;
    std::vector<int> dsk = {1, 1};
    std::vector<int> nextkey = {0};
    std::string transition_type = "";
    int bkgd = 1;
    int prvtrans = 1;
    // 打印数据帧
    std::vector<unsigned char> format_frame1(const std::vector<unsigned char> &data);
    std::vector<unsigned char> format_frame2(const std::vector<unsigned char> &data);
};
};// namespace seeder


// #endif // UART_SEND_H
