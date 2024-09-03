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

namespace seeder{
/* 定义 CAN 接口名称和帧 ID */
#define CAN_INTERFACE "can0"
#define FRAME1_ID 0x602 // 主站can消息
#define FRAME2_ID 0x604 // 从站1
#define FRAME3_ID 0x606 // 从站2
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

    // 获取套接字文件描述符
    int getSocketFd() const;

private:
    std::string interface_name; // CAN 接口名称
    int socket_fd;              // 套接字文件描述符

    // 设置并初始化 CAN 套接字
    void setupSocket();

    // 设置套接字为非阻塞模式
    void setNonblocking();

};//class CanSocket

class CanFrameProcessor
{
public:
    // 构造函数，接受 CanSocket 实例和 Switcher 实例
    CanFrameProcessor(CanSocket &canSocket, std::shared_ptr<seeder::Switcher> sw);

    // 启动 CAN 帧处理线程
    void startProcessing();

    // 停止 CAN 帧处理线程
    void stopProcessing();

private:
    CanSocket &socket;                    // 引用 CanSocket 实例
    std::thread processing_thread;        // 处理线程
    std::atomic<bool> stop_flag{false};  // 停止标志
    std::shared_ptr<seeder::Switcher> switcher;
    int dsk = 0;
    int64_t dsk_excut_time = 0;
    int idx = 0;
    int pgm = 0;
    int pvw = 0;
    int nextkey = 0;
    int proxy_key_source = 0;

    // 处理 CAN 帧数据
    void processFrames();

    // 处理特定 ID 的 CAN 帧数据
    void processFrame(const can_frame &frame);

    // 处理并发送 CAN 帧数据 -- 推子板
    void handle_FrameData1(const can_frame &frame, int frame_id);

    // 处理并发送 CAN 帧数据 -- 从站1
    void handle_FrameData2(const can_frame &frame, int frame_id);

    // 处理并发送 CAN 帧数据 -- 从站2
    void handle_FrameData3(const can_frame &frame, int frame_id);

    // 计算按键索引的辅助函数
    int calculateidex(unsigned char value, int offset);

    // 打印 CAN 帧数据
    void printCanFrame(const can_frame &frame, const std::string &label);

    // 打印 CAN 帧数据（简化版）
    void printFrameData(const can_frame &frame);

};//class CanFrameProcessor


};//naemspace seeder
// #endif // CAN_RECEIVE_H