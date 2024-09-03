#include "src/switcher.h"
#include <atomic>
#include <boost/program_options.hpp>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <memory>
#include <net/if.h>
#include <numeric>
#include <stdexcept>
#include <sys/socket.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <ctime>

/* 定义 CAN 接口名称和帧 ID */
#define CAN_INTERFACE "can0"
#define FRAME1_ID 0x602 // 主站can消息
#define FRAME2_ID 0x604 // 从站1
#define FRAME3_ID 0x606 // 从站2
#define FRAME4_ID 0x608 // 从站3
#define FRAME5_ID 0x610 // 从站4
#define FRAME6_ID 0x612 // 从站5
#define FRAME7_ID 0x614 // 从站6

// 定义数据帧大小
constexpr size_t SCREEN_DATA_SIZE = 40; // 每个屏幕数据大小
constexpr int NUM_SCREENS = 3;          // 屏幕数量
constexpr int NUM_SLAVES = 6;           // 从机数量
constexpr size_t FRAME1_SIZE =
    1 + 1 + NUM_SCREENS * SCREEN_DATA_SIZE * NUM_SLAVES + 1 + 1; // 第一帧数据大小

constexpr size_t MAIN_CTRL_KEYS_SIZE = 25;                                                         // 主控板按键灯光数据大小
constexpr size_t SLAVE_KEYS_SIZE = 24;                                                             // 从机按键灯光数据大小
constexpr int NUM_SLAVE_KEYS = 6;                                                                  // 从机数量
constexpr size_t FRAME2_SIZE = 1 + 1 + MAIN_CTRL_KEYS_SIZE + NUM_SLAVE_KEYS * SLAVE_KEYS_SIZE + 1 + 1; // 第二帧数据大小
int idx = 0;
int Pgm, Pvw = 0;
int MIX, CUT, AUTO = 0;
int idex = 0;
int pgm_flag = 0;
int pvw_flag = 0;
int DSK0, DSK1 = 0;

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
  explicit UART(const UARTConfig &config)
      : port_(config.port), baud_rate_(config.baud_rate), fd_(-1)
  {
    openPort();  // 打开串口
    configure(); // 配置串口参数
  }

  ~UART()
  {
    closePort(); // 析构时关闭串口
  }

  UART(const UART &) = delete;            // 禁止拷贝构造函数
  UART &operator=(const UART &) = delete; // 禁止拷贝赋值运算符

  // 发送数据帧
  void sendFrame(const std::vector<unsigned char> &data)
  {
    if (fd_ == -1)
    {
      throw std::runtime_error("UART端口未打开");
    }
    tcflush(fd_, TCIOFLUSH);                                      // 清空UART缓冲区
    ssize_t bytes_written = write(fd_, data.data(), data.size()); // 写入数据
    if (bytes_written < 0)
    {
      throw std::runtime_error("写入数据失败");
    }
    else if (static_cast<size_t>(bytes_written) != data.size())
    {
      std::cerr << "警告：发送的数据长度与预期长度不符" << std::endl;
    }
  }

private:
  int fd_;            // 文件描述符
  std::string port_;  // 串口端口
  speed_t baud_rate_; // 波特率

  // 打开串口
  void openPort()
  {
    fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ == -1)
    {
      throw std::runtime_error("无法打开UART端口: " + port_);
    }
  }

  // 关闭串口
  void closePort()
  {
    if (fd_ != -1)
    {
      close(fd_);
      fd_ = -1;
    }
  }

  // 配置串口参数
  void configure()
  {
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd_, &tty) != 0)
    {
      throw std::runtime_error("tcgetattr() 错误");
    }
    cfsetospeed(&tty, baud_rate_);
    cfsetispeed(&tty, baud_rate_);
    tty.c_cflag |= (CLOCAL | CREAD); // 本地连接，启用接收
    tty.c_cflag &= ~PARENB;          // 无奇偶校验
    tty.c_cflag &= ~CSTOPB;          // 1位停止位
    tty.c_cflag &= ~CSIZE;           // 清除数据位设置
    tty.c_cflag |= CS8;              // 8位数据位
    if (tcsetattr(fd_, TCSANOW, &tty) != 0)
    {
      throw std::runtime_error("tcsetattr() 错误");
    }
  }
};

// 数据帧类，用于构造数据帧
class DataFrame
{
public:
  DataFrame()
  {
    std::srand(static_cast<unsigned int>(std::time(nullptr))); // 设置随机种子
  }

  // 构造第一帧数据
  std::vector<unsigned char> construct_frame1(unsigned char frame_header,
                                              unsigned char device_id)
  {
    std::vector<unsigned char> frame(FRAME1_SIZE, 0); // 初始化数据帧
    size_t offset = 0;
    frame[offset++] = frame_header; // 帧头
    frame[offset++] = device_id;    // 设备ID

    // 填充屏幕数据
    for (int slave = 0; slave < NUM_SLAVES; ++slave)
    {
      if (slave == 0)
      {
        for (int screen = 0; screen < NUM_SCREENS; ++screen)
        {
          // 初始化字节数据
          if (screen == 0)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X31, 0X4B, 0X45, 0X59, 0X30, 0X32, 0X4B, 0X45, 0X59, 0X30, 0X33, 0X4B, 0X45, 0X59, 0X30, 0X34, /*KEY1-4*/
                0X43, 0X41, 0X4D, 0X30, 0X31, 0X43, 0X41, 0X4D, 0X30, 0X32, 0X43, 0X41, 0X4D, 0X30, 0X33, 0X43, 0X41, 0X4D, 0X30, 0X34  /*CAM1-4*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 1)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X35, 0X4B, 0X45, 0X59, 0X30, 0X36, 0X4B, 0X45, 0X59, 0X30, 0X37, 0X4B, 0X45, 0X59, 0X30, 0X38, /*KEY5-8*/
                0X43, 0X41, 0X4D, 0X30, 0X35, 0X43, 0X41, 0X4D, 0X30, 0X36, 0X43, 0X41, 0X4D, 0X30, 0X37, 0X43, 0X41, 0X4D, 0X30, 0X38  /*CAM5-8*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 2)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X39, 0X4B, 0X45, 0X59, 0X31, 0X30, 0X4B, 0X45, 0X59, 0X31, 0X31, 0X4B, 0X45, 0X59, 0X31, 0X32, /*KEY9-12*/
                0X43, 0X41, 0X4D, 0X30, 0X39, 0X43, 0X41, 0X4D, 0X31, 0X30, 0X43, 0X41, 0X4D, 0X31, 0X31, 0X43, 0X41, 0X4D, 0X31, 0X32  /*CAM9-12*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          offset += SCREEN_DATA_SIZE;
        }
      }
      if (slave == 1)
      {
        for (int screen = 0; screen < NUM_SCREENS; ++screen)
        {
          // 初始化字节数据
          if (screen == 0)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X31, 0X50, 0X47, 0X4D, 0X30, 0X32, 0X50, 0X47, 0X4D, 0X30, 0X33, 0X50, 0X47, 0X4D, 0X30, 0X34, /*PGM1-4*/
                0X50, 0X56, 0X57, 0X30, 0X31, 0X50, 0X56, 0X57, 0X30, 0X32, 0X50, 0X56, 0X57, 0X30, 0X33, 0X50, 0X56, 0X57, 0X30, 0X34  /*PVW1-4*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 1)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X35, 0X50, 0X47, 0X4D, 0X30, 0X36, 0X50, 0X47, 0X4D, 0X30, 0X37, 0X50, 0X47, 0X4D, 0X30, 0X38, /*PGM5-8*/
                0X50, 0X56, 0X57, 0X30, 0X35, 0X50, 0X56, 0X57, 0X30, 0X36, 0X50, 0X56, 0X57, 0X30, 0X37, 0X50, 0X56, 0X57, 0X30, 0X38  /*PVW5-8*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 2)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X39, 0X50, 0X47, 0X4D, 0X31, 0X30, 0X50, 0X47, 0X4D, 0X31, 0X31, 0X50, 0X47, 0X4D, 0X31, 0X32, /*PGM9-12*/
                0X50, 0X56, 0X57, 0X30, 0X39, 0X50, 0X56, 0X57, 0X31, 0X30, 0X50, 0X56, 0X57, 0X31, 0X31, 0X50, 0X56, 0X57, 0X31, 0X32  /*PVW9-12*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          offset += SCREEN_DATA_SIZE;
        }
      }
      if (slave == 2 )
      {
        for (int screen = 0; screen < NUM_SCREENS; ++screen)
        {
          // 初始化字节数据
          if (screen == 0)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X31, 0X4B, 0X45, 0X59, 0X30, 0X32, 0X4B, 0X45, 0X59, 0X30, 0X33, 0X4B, 0X45, 0X59, 0X30, 0X34, /*KEY1-4*/
                0X43, 0X41, 0X4D, 0X30, 0X31, 0X43, 0X41, 0X4D, 0X30, 0X32, 0X43, 0X41, 0X4D, 0X30, 0X33, 0X43, 0X41, 0X4D, 0X30, 0X34  /*CAM1-4*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 1)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X35, 0X4B, 0X45, 0X59, 0X30, 0X36, 0X4B, 0X45, 0X59, 0X30, 0X37, 0X4B, 0X45, 0X59, 0X30, 0X38, /*KEY5-8*/
                0X43, 0X41, 0X4D, 0X30, 0X35, 0X43, 0X41, 0X4D, 0X30, 0X36, 0X43, 0X41, 0X4D, 0X30, 0X37, 0X43, 0X41, 0X4D, 0X30, 0X38  /*CAM5-8*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 2)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X39, 0X4B, 0X45, 0X59, 0X31, 0X30, 0X4B, 0X45, 0X59, 0X31, 0X31, 0X4B, 0X45, 0X59, 0X31, 0X32, /*KEY9-12*/
                0X43, 0X41, 0X4D, 0X30, 0X39, 0X43, 0X41, 0X4D, 0X31, 0X30, 0X43, 0X41, 0X4D, 0X31, 0X31, 0X43, 0X41, 0X4D, 0X31, 0X32  /*CAM9-12*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          offset += SCREEN_DATA_SIZE;
        }
      }
      if (slave == 3)
      {
        for (int screen = 0; screen < NUM_SCREENS; ++screen)
        {
          // 初始化字节数据
          if (screen == 0)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X31, 0X50, 0X47, 0X4D, 0X30, 0X32, 0X50, 0X47, 0X4D, 0X30, 0X33, 0X50, 0X47, 0X4D, 0X30, 0X34, /*PGM1-4*/
                0X50, 0X56, 0X57, 0X30, 0X31, 0X50, 0X56, 0X57, 0X30, 0X32, 0X50, 0X56, 0X57, 0X30, 0X33, 0X50, 0X56, 0X57, 0X30, 0X34  /*PVW1-4*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 1)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X35, 0X50, 0X47, 0X4D, 0X30, 0X36, 0X50, 0X47, 0X4D, 0X30, 0X37, 0X50, 0X47, 0X4D, 0X30, 0X38, /*PGM5-8*/
                0X50, 0X56, 0X57, 0X30, 0X35, 0X50, 0X56, 0X57, 0X30, 0X36, 0X50, 0X56, 0X57, 0X30, 0X37, 0X50, 0X56, 0X57, 0X30, 0X38  /*PVW5-8*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 2)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X39, 0X50, 0X47, 0X4D, 0X31, 0X30, 0X50, 0X47, 0X4D, 0X31, 0X31, 0X50, 0X47, 0X4D, 0X31, 0X32, /*PGM9-12*/
                0X50, 0X56, 0X57, 0X30, 0X39, 0X50, 0X56, 0X57, 0X31, 0X30, 0X50, 0X56, 0X57, 0X31, 0X31, 0X50, 0X56, 0X57, 0X31, 0X32  /*PVW9-12*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          offset += SCREEN_DATA_SIZE;
        }
      }
      if (slave == 4 )
      {
        for (int screen = 0; screen < NUM_SCREENS; ++screen)
        {
          // 初始化字节数据
          if (screen == 0)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X31, 0X4B, 0X45, 0X59, 0X30, 0X32, 0X4B, 0X45, 0X59, 0X30, 0X33, 0X4B, 0X45, 0X59, 0X30, 0X34, /*KEY1-4*/
                0X43, 0X41, 0X4D, 0X30, 0X31, 0X43, 0X41, 0X4D, 0X30, 0X32, 0X43, 0X41, 0X4D, 0X30, 0X33, 0X43, 0X41, 0X4D, 0X30, 0X34  /*CAM1-4*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 1)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X35, 0X4B, 0X45, 0X59, 0X30, 0X36, 0X4B, 0X45, 0X59, 0X30, 0X37, 0X4B, 0X45, 0X59, 0X30, 0X38, /*KEY5-8*/
                0X43, 0X41, 0X4D, 0X30, 0X35, 0X43, 0X41, 0X4D, 0X30, 0X36, 0X43, 0X41, 0X4D, 0X30, 0X37, 0X43, 0X41, 0X4D, 0X30, 0X38  /*CAM5-8*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 2)
          {
            unsigned char bytes[] = {
                0X4B, 0X45, 0X59, 0X30, 0X39, 0X4B, 0X45, 0X59, 0X31, 0X30, 0X4B, 0X45, 0X59, 0X31, 0X31, 0X4B, 0X45, 0X59, 0X31, 0X32, /*KEY9-12*/
                0X43, 0X41, 0X4D, 0X30, 0X39, 0X43, 0X41, 0X4D, 0X31, 0X30, 0X43, 0X41, 0X4D, 0X31, 0X31, 0X43, 0X41, 0X4D, 0X31, 0X32  /*CAM9-12*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          offset += SCREEN_DATA_SIZE;
        }
      }
      if (slave == 5)
      {
        for (int screen = 0; screen < NUM_SCREENS; ++screen)
        {
          // 初始化字节数据
          if (screen == 0)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X31, 0X50, 0X47, 0X4D, 0X30, 0X32, 0X50, 0X47, 0X4D, 0X30, 0X33, 0X50, 0X47, 0X4D, 0X30, 0X34, /*PGM1-4*/
                0X50, 0X56, 0X57, 0X30, 0X31, 0X50, 0X56, 0X57, 0X30, 0X32, 0X50, 0X56, 0X57, 0X30, 0X33, 0X50, 0X56, 0X57, 0X30, 0X34  /*PVW1-4*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 1)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X35, 0X50, 0X47, 0X4D, 0X30, 0X36, 0X50, 0X47, 0X4D, 0X30, 0X37, 0X50, 0X47, 0X4D, 0X30, 0X38, /*PGM5-8*/
                0X50, 0X56, 0X57, 0X30, 0X35, 0X50, 0X56, 0X57, 0X30, 0X36, 0X50, 0X56, 0X57, 0X30, 0X37, 0X50, 0X56, 0X57, 0X30, 0X38  /*PVW5-8*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          if (screen == 2)
          {
            unsigned char bytes[] = {
                0X50, 0X47, 0X4D, 0X30, 0X39, 0X50, 0X47, 0X4D, 0X31, 0X30, 0X50, 0X47, 0X4D, 0X31, 0X31, 0X50, 0X47, 0X4D, 0X31, 0X32, /*PGM9-12*/
                0X50, 0X56, 0X57, 0X30, 0X39, 0X50, 0X56, 0X57, 0X31, 0X30, 0X50, 0X56, 0X57, 0X31, 0X31, 0X50, 0X56, 0X57, 0X31, 0X32  /*PVW9-12*/
            };
            std::copy(std::begin(bytes), std::end(bytes), frame.data() + offset);
          }
          offset += SCREEN_DATA_SIZE;
        }
      }
    }
    // 结束位
    frame[offset++] = 0x00;
    frame[offset] = calculate_checksum(frame, 2, FRAME1_SIZE - 1); // 计算校验和
    return frame;
  };

  std::vector<unsigned char> construct_frame3(unsigned char frame_header,
                                              unsigned char device_id, int pgm,
                                              int pvw, std::string transition_type, std::vector<int> dsk)
  {
    std::vector<unsigned char> frame(FRAME2_SIZE, 0); // 初始化数据帧
    size_t offset = 0;
    frame[offset++] = frame_header; // 帧头
    frame[offset++] = device_id;    // 设备ID

    unsigned char main_ctrl_keys_bytes[MAIN_CTRL_KEYS_SIZE] = {0};

    for (int i = 0; i < MAIN_CTRL_KEYS_SIZE; i++)
    {
      main_ctrl_keys_bytes[i] = 1;
    };
    if (transition_type == "mix")
    {
      main_ctrl_keys_bytes[10] = 2;
    }
    if (!dsk.empty())
    {
      for (size_t i = 0; i < dsk.size(); i++)
      {
        main_ctrl_keys_bytes[17 + i] = dsk[i];
      }
    }
    std::memcpy(frame.data() + offset, main_ctrl_keys_bytes,
                MAIN_CTRL_KEYS_SIZE);
    offset += MAIN_CTRL_KEYS_SIZE;

    unsigned char slave_keys[NUM_SLAVE_KEYS][SLAVE_KEYS_SIZE] = {0};
    for (int i = 0; i < NUM_SLAVE_KEYS; i++)
    {
      for (int j = 0; j < SLAVE_KEYS_SIZE; j++)
      {
        slave_keys[i][j] = 1;
      }
    }
    slave_keys[3][pgm - 1] = 2;
    slave_keys[3][12 + pvw - 1] = 3;
    for (int i = 0; i < 6; i++)
    {
      std::memcpy(frame.data() + offset, slave_keys[i], SLAVE_KEYS_SIZE);
      offset += SLAVE_KEYS_SIZE;
    }
    frame[offset] = 0x00; // 结束位
    // 添加帧尾部（1字节校验和）
    frame[offset+1] = calculate_checksum(frame, 2, FRAME2_SIZE - 1);
    return frame;
  };

private:
  // 计算数据的校验和
  unsigned char calculate_checksum(const std::vector<unsigned char> &data,
                                   size_t start, size_t end)
  {
    unsigned char sum = 0;
    for (size_t i = start; i < end; ++i)
    {
      sum += data[i];
    }
    return sum;
  }
};

// 主控制类
class MainController
{
public:
  MainController(const UARTConfig &uart_config1, const UARTConfig &uart_config3)
      : uart1_(uart_config1), uart3_(uart_config3), data_frame_(), stop_flag_(false) {
      }

  // 接收服务器的消息，构造并发送数据帧
  // DSK
  void handler_dsk_status(std::vector<bool> dsk_status)
  {
    // resetFlags();
    bool sendFlag = false;

    for (size_t i = 0; i < this->dsk.size(); i++)
    {
      if (i < dsk_status.size())
      {
        int status = dsk_status[i] ? 2 : 1;
        if (status != this->dsk[i])
        {
          sendFlag = true;
          this->dsk[i] = status;
        }
      }
    }

    // for (int i = 0; i < dsk_status.size(); i++)
    // {
    //   printf("dsk[%i]: %s \n", i, dsk_status[i] ? "true" : "false");
    // }
    if (sendFlag)
    {
      sendFrames();
    }
  }
  // MIX
  void handler_transition_type(std::string transition_type)
  {
    if (this->transition_type != transition_type)
    {
      this->transition_type = transition_type;
      sendFrames();
    }
  }
  // PGM PVW
  void handler_status(int pgm_, int pvw_)
  { /*3  2*/
    // resetFlags();
    pgm_ += 1; /*4*/
    pvw_ += 1; /*3*/
    // printf("receive pgm : %i , pvw : %i \n", pgm_, pvw_);
    if (pgm_ == this->pgm && pvw_ == this->pvw)
    {
      return;
    }
    if (pgm_ != this->pgm)
    {
      this->pgm = pgm_;
    }
    if (pvw_ != this->pvw)
    {
      this->pvw = pvw_;
    }
    sendFrames();
  }
  // 停止接收
  void stop()
  {
    stop_flag_.store(true); // 设置停止标志
    if (sender_thread_.joinable())
    {
      sender_thread_.join(); // 等待发送线程结束
    }
    if (printer_thread_.joinable())
    {
      printer_thread_.join(); // 等待打印线程结束
    }
  }

  void sendFrames()
  {
    auto frame1 = data_frame_.construct_frame1(0xA8, 0x01);
    auto frame2 = data_frame_.construct_frame3(0xB7, 0x01, this->pgm, this->pvw, this->transition_type, this->dsk);
    try
    {
      uart1_.sendFrame(frame1); // 发送第一帧
      usleep(50000);            // 50毫秒

      uart3_.sendFrame(frame2); // 发送第二帧
      usleep(50000);            // 50毫秒
    }
    catch (const std::exception &e)
    {
      std::cerr << "发送错误: " << e.what() << std::endl;
    }
    std::cout << "第一帧数据:" << std::endl;
    print_frame(frame1);
    std::cout << "第二帧数据:" << std::endl;
    print_frame(frame2);
  }
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
  std::string transition_type = "";

  void resetFlags()
  {
    idex = 0;
    pgm_flag = 0;
    pvw_flag = 0;
    DSK0 = 0;
    DSK1 = 0;
    MIX = 0;
  }


  // 打印数据帧
  void print_frame(const std::vector<unsigned char> &data)
  {
    for (size_t i = 0; i < data.size(); ++i)
    {
      std::cout << std::hex << std::uppercase << static_cast<int>(data[i])
                << " ";
      if ((i + 1) % 16 == 0)
      {
        std::cout << std::endl;
      }
    }
    std::cout << std::endl;
  }
};

// 类用于管理 CAN 套接字
class CanSocket
{
public:
  // 构造函数，初始化 CAN 套接字
  CanSocket(const std::string &interface) : interface_name(interface)
  {
    setupSocket();
  }

  // 析构函数，关闭 CAN 套接字
  ~CanSocket() { close(socket_fd); }

  // 获取套接字文件描述符
  int getSocketFd() const { return socket_fd; }

private:
  std::string interface_name; // CAN 接口名称
  int socket_fd;              // 套接字文件描述符

  // 设置并初始化 CAN 套接字
  void setupSocket()
  {
    struct sockaddr_can addr;

    // 创建 raw CAN 套接字
    if ((socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
      std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
      throw std::runtime_error("Socket creation failed");
    }

    // 初始化 sockaddr_can 结构体
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(interface_name.c_str());

    if (addr.can_ifindex == 0)
    {
      std::cerr << "Invalid CAN interface: " << strerror(errno) << std::endl;
      throw std::runtime_error("Invalid CAN interface");
    }

    // 绑定套接字到 CAN 接口
    if (bind(socket_fd, reinterpret_cast<struct sockaddr *>(&addr),
             sizeof(addr)) < 0)
    {
      std::cerr << "Bind failed: " << strerror(errno) << std::endl;
      throw std::runtime_error("Bind failed");
    }

    // 设置套接字为非阻塞模式
    setNonblocking();
  }

  // 设置套接字为非阻塞模式
  void setNonblocking()
  {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0)
    {
      throw std::runtime_error("fcntl F_GETFL failed");
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      throw std::runtime_error("fcntl F_SETFL failed");
    }
  }
};

// 类用于处理 CAN 帧
class CanFrameProcessor
{
public:
  // 构造函数，接受 CanSocket 实例
  CanFrameProcessor(CanSocket &canSocket, std::shared_ptr<seeder::Switcher> sw)
      : socket(canSocket), switcher(sw), stop_flag(false) {}

  // 启动 CAN 帧处理线程
  void startProcessing()
  {
    processing_thread = std::thread(&CanFrameProcessor::processFrames, this);
  }

  // 停止 CAN 帧处理线程
  void stopProcessing()
  {
    stop_flag = true;
    if (processing_thread.joinable())
    {
      processing_thread.join();
    }
  }

private:
  CanSocket &socket;                  // 引用 CanSocket 实例
  std::thread processing_thread;      // 处理线程
  std::atomic<bool> stop_flag{false}; // 停止标志
  std::shared_ptr<seeder::Switcher> switcher;
  int dsk = 0;
  int64_t dsk_excut_time = 0;

  int pgm = 0;
  int pvw = 0;
  // 处理 CAN 帧数据
  void processFrames()
  {
    can_frame frame;
    int nbytes;

    while (!stop_flag)
    {
      memset(&frame, 0, sizeof(can_frame));
      nbytes = read(socket.getSocketFd(), &frame, sizeof(can_frame));

      if (nbytes > 0)
      {
        // 处理 CAN 帧数据
        processFrame(frame);
      }
      else if (nbytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
      {
        std::cerr << "Read failed: " << strerror(errno) << std::endl;
      }
    }
  }

  void processFrame(const can_frame &frame)
  {
      printf("receive can frame_id : %x \n", frame.can_id);
    switch (frame.can_id)
    {
    case FRAME1_ID:
     // printf("receive can frame_id : %x \n", frame.can_id);
      // handleFrameData(frame, "buffer1", handle_FrameData1);
      handle_FrameData1(frame, FRAME1_ID);
      break;
    case FRAME2_ID:
      handle_FrameData2(frame, FRAME2_ID);
      break;
    case FRAME5_ID:
       // printf("receive can frame : %x \n", frame.can_id);
      handle_FrameData3(frame, FRAME5_ID);
      break;
    }
  }

  void handleFrameData(const can_frame &frame, const std::string &label,
                       std::function<void(const can_frame &, int)> handler)
  {
    handler(frame, 1);
    printCanFrame(frame, label);
  };

  // 处理并发送 CAN 帧数据 -- 推子板
  void handle_FrameData1(const can_frame &frame, int frame_id)
  {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    // 检查帧 ID 和数据有效性
    // MIX = 0;CUT = 0;AUTO = 0;
    if (frame.data[0] != 0x00)
    {
      idx = calculateidex(frame.data[0], 1);
    }
    else if (frame.data[1] != 0x00)
    {
      idx = calculateidex(frame.data[1], 9);
      if (idx == 11)
      {
        // MIX = 1;
        switcher->mix();
      }
      else if (idx == 16)
      {
        // AUTO = 1;
        switcher->auto_();
      }
    }
    else if (frame.data[2] != 0x00)
    {
      idx = calculateidex(frame.data[2], 17);
      if (idx == 17)
      {
        // CUT = 1;
        switcher->cut();
      }
      else if (idx == 18)
      {
        if (milliseconds - dsk_excut_time > 1000)
        {
          printf("receive can dsk : %x \n", idx);
          switcher->dsk(0);
        }
      }
      if (idx == 19)
      {

        if (milliseconds - dsk_excut_time > 1000)
        {
          printf("receive can dsk : %x \n", idx);
          switcher->dsk(1);
        }
      }
    }
    else
    {
      idx = 25;
    }
  }

  // 处理并发送 CAN 帧数据 -- 从站1
  void handle_FrameData2(const can_frame &frame, int frame_id)
  {
    // Pgm = 1;
    if (frame.data[0] != 0x00)
    {
      idx = calculateidex(frame.data[0], 1);
      // switcher->pgm(idx);
    }
    else if (frame.data[1] != 0x00)
    {
      idx = calculateidex(frame.data[1], 9);
      // switcher->pgm(idx);
    }
    else if (frame.data[2] != 0x00)
    {
      idx = calculateidex(frame.data[2], 17);
      // switcher->pgm(idx);
    }
  }
  void printFrameData(const can_frame &frame)
  {
    printf("recive can frame :");
    for (size_t i = 0; i < frame.can_dlc; i++)
    {
      printf(" %x ", frame.data[i]);
    }
    printf("\n");
  }
  // 处理并发送 CAN 帧数据 -- 从站2
  void handle_FrameData3(const can_frame &frame, int frame_id)
  {
    // printFrameData(frame);
    if (frame.data[0] != 0x00)
    {
      idx = calculateidex(frame.data[0], 1);
      if (this->pgm == idx)
      {
        return;
      }
      this->pgm = idx;
      switcher->pgm(this->pgm);
      printFrameData(frame);
      printf("第一帧：pgm: %d\n", this->pgm);
    }
    else if (frame.data[1] != 0x00)
    {
      idx = calculateidex(frame.data[1], 9);
      if (idx < 13)
      {
        if (this->pgm == idx)
        {
          return;
        }
        this->pgm = idx;
        switcher->pgm(this->pgm);
        printFrameData(frame);
        printf("第二帧：pgm: %d\n", this->pgm);
      }
      else
      {
        if (this->pvw == idx)
        {
          return;
        }
        this->pvw = idx;
        // Pvw = 1;
        switcher->pvw(this->pvw - 12);
        printFrameData(frame);
        printf("第二帧：pvw: %d\n", this->pvw);
      }
      // switcher->pgm(idx);
    }
    else if (frame.data[2] != 0x00)
    {
      idx = calculateidex(frame.data[2], 17);
      if (this->pvw == idx)
      {
        return;
      }
      this->pvw = idx;
      switcher->pvw(this->pvw - 12);
      printFrameData(frame);
      printf("第三帧：pvw: %d\n", this->pvw);
    }
  }

  // 计算按键索引的辅助函数
  int calculateidex(unsigned char value, int offset)
  {
    for (int i = 0; i < 8; ++i)
    {
      if (value & (1 << i))
      {
        return offset + i;
      }
    }
    return -1; // 默认返回值
  }

  // 打印 CAN 帧数据
  void printCanFrame(const can_frame &frame, const std::string &label)
  {
    std::cout << "Frame from " << label << ": ID: " << std::hex << frame.can_id
              << "  DLC: " << std::dec << static_cast<int>(frame.can_dlc) << "  Data: ";
    for (int i = 0; i < frame.can_dlc; ++i)
    {
      std::cout << std::hex << static_cast<int>(frame.data[i]) << " "; // 打印数据
    }
    std::cout << std::endl;
  }
};

// 主函数
int main(int argc, char **argv)
{
  // 读取配置文件
  namespace po = boost::program_options;
  std::string config_path;
  po::options_description desc("Seeder Switcher Keybaord");
  desc.add_options()("config_file", po::value(&config_path)->required(),
                     "main config file");
  po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(desc).run();
  po::variables_map po_vm;
  po::store(parsed, po_vm);
  po::notify(po_vm);

  try
  {
    std::shared_ptr<seeder::Switcher> switcher =
        std::make_shared<seeder::Switcher>(config_path);
    switcher->init();
    UARTConfig config1{"/dev/ttyS1"}; // 串口1配置
    UARTConfig config3{"/dev/ttyS3"};
    //  UARTConfig config3{} ;          // 串口3配置
    MainController controller(config1, config3); // 创建主控制器对象
    switcher->set_panel_status_handler(
        std::bind(&MainController::handler_status, &controller,
                  std::placeholders::_1, std::placeholders::_2));
    switcher->set_dsk_status_handler(std::bind(&MainController::handler_dsk_status,
                                               &controller, std::placeholders::_1));
    switcher->set_transition_type_handler(std::bind(
        &MainController::handler_transition_type, &controller,
        std::placeholders::_1));
    CanSocket canSocket(CAN_INTERFACE); // 创建 CAN 套接字实例
    CanFrameProcessor frameProcessor(canSocket,
                                     switcher); // 创建 CAN 帧处理实例
    frameProcessor.startProcessing();           // 启动 CAN 帧接收处理
    printf("Pgm = %d,Pvw = %d,MIX = %d,CUT = %d,AUTO = %d,idx = %d\n", Pgm, Pvw, MIX, CUT,
           AUTO, idx);
    usleep(1000000); // 等待 1 秒
    controller.sendFrames();
    // std::cin.get(); // 等待用户输入
    while (true)
    {
    }
    controller.stop();               // 停止串口控制器的线程
    frameProcessor.stopProcessing(); // 停止 CAN 帧接收处理
    std::cout << "程序已停止" << std::endl;
  }
  catch (const std::runtime_error &e)
  {
    std::cerr << "运行时错误: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}