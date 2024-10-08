#include "uart_send.h"
#include <cstring>

namespace seeder {

UART::UART(const UARTConfig &config) : port_(config.port), baud_rate_(config.baud_rate), fd_(-1){
    openPort();  // 打开串口
    configure(); // 配置串口参数
}

UART::~UART() {
    stop();
}

void UART::openPort() {
    fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ == -1){
        logger->error("Unable to open UART port: {}", port_);
    }
}

void UART::closePort() {
    if (fd_ != -1){
        close(fd_);
        fd_ = -1;
    }
}

// 配置串口参数
void UART::configure() {
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd_, &tty) != 0){
        logger->error("tcgetattr() error: {}", strerror(errno));
    }
    cfsetospeed(&tty, baud_rate_);
    cfsetispeed(&tty, baud_rate_);
    tty.c_cflag &= ~CRTSCTS;          // 关闭硬件流控
    tty.c_cflag |= (CLOCAL | CREAD);  // 本地连接，启用接收
    tty.c_cflag &= ~(PARENB | PARODD);// 无奇偶校验
    tty.c_cflag &= ~CSTOPB;           // 1位停止位
    tty.c_cflag &= ~CSIZE;            // 清除数据位设置
    tty.c_cflag |= CS8;               // 8位数据位
    if (tcsetattr(fd_, TCSANOW, &tty) != 0){
        logger->error("tcsetattr() error: {}", strerror(errno));
    }
}

void UART::sendFrame(const std::vector<unsigned char> &data)
{
    if (fd_ == -1){
        logger->error("UART port is not open");
    }
    tcflush(fd_, TCIOFLUSH);                              // 清空UART缓冲区
    ssize_t bytes_written = write(fd_, data.data(), data.size()); // 写入数据
    if (bytes_written < 0){
        logger->error("Failed to write data to UART: {}", strerror(errno));
    }
    else if (static_cast<size_t>(bytes_written) != data.size()){
        logger->warn("Sent data length does not match expected length");
    }
}

void UART::init() {
    openPort();
    configure();
}
void UART::start() {
    // Any additional startup procedures if needed
}
void UART::stop() {
    closePort();
}

// 函数：从文件中读取十六进制数据并解析为 6x3x40 的三维数组
std::vector<std::vector<std::vector<std::string>>> DataFrame::readHexData(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::in);  // 打开文件进行读取
    // 初始化 6x3x40 的三维数组
    std::vector<std::vector<std::vector<std::string>>> data(NUM_SLAVES, std::vector<std::vector<std::string>>(NUM_SCREENS, std::vector<std::string>(SCREEN_DATA_SIZE, ""))); 
    std::string line;                // 存储每行读取的数据
    int layerIndex = 0;              // 当前站索引
    int rowIndex = 0;                // 当前屏幕索引
    int colIndex = 0;                // 当前列索引

    // 检查文件是否成功打开
    if (!ifs.is_open()) {
        logger->error("Error opening file: {}", filename);
        return data;
    }

    // 逐行读取文件内容
    while (getline(ifs, line)) {
        std::stringstream ss(line);  // 用字符串流处理每行数据
        std::string hexStr;          // 存储当前解析的十六进制字符串
        while (ss >> hexStr) {  // 逐个读取十六进制字符串
            // 确保十六进制字符串正确，包含 "0x" 前缀
            if (hexStr.length() > 2 && hexStr.substr(0, 2) == "0x") {
                // 将十六进制字符串直接存储到数组中
                if (layerIndex < NUM_SLAVES && rowIndex < NUM_SCREENS && colIndex < SCREEN_DATA_SIZE) {
                    data[layerIndex][rowIndex][colIndex] = hexStr;
                    colIndex++;
                    // 当一列填满后，移动到下一行
                    if (colIndex >= SCREEN_DATA_SIZE) {
                        colIndex = 0;
                        rowIndex++;
                        // 当一行填满后，移动到下一层
                        if (rowIndex >= NUM_SCREENS) {
                            rowIndex = 0;
                            layerIndex++;
                        }
                    }
                }
            }
        }
        // 如果已填满整个三维数组，则退出循环
        if (layerIndex >= NUM_SLAVES) {
            break;
        }
    }

    ifs.close();  // 关闭文件
    return data;  // 返回读取的 6x3x40 三维数组
}

// //对OLED显示数据文件进行操作
// std::vector<std::vector<std::vector<std::string>>> DataFrame::file_event_handlers() {
//     const std::string filename = "/opt/switcher-keyboard/datas/data.txt";  // 文件路径（请根据实际文件路径修改）

//     // 从文件中读取十六进制数据并解析为 6x3x40 三维数组
//     std::vector<std::vector<std::vector<std::string>>> data = readHexData(filename);

//     // // 修改OLED显示屏数据
//     // if (data[0][0].size() > 4) {  // 确保数组足够大
//     //     data[0][0][4] = "0xAB";  // 修改为新的十六进制值，例如 "0xAB"
//     // }

//     // 将修改后的数据写回到文件中
//     // writeHexData(filename, data);

//     return data;
// }

// 构造第一帧数据 -- OLED显示屏数据
std::vector<unsigned char> DataFrame::construct_frame1(
    unsigned char frame_header,
    unsigned char device_id,
    const std::vector<std::vector<std::vector<std::string>>> &data)
{
    // std::vector<unsigned char> frame(FRAME1_SIZE, 0); // 初始化数据帧
    std::vector<unsigned char> frame;
    // frame.clear();
    frame.reserve(FRAME1_SIZE); // 预分配内存以提高效率
    frame.push_back(frame_header);// 帧头
    frame.push_back(device_id);// 设备ID
    // 将三维数组的数据转换为 unsigned char 并存储到 vector 中
    for (int i = 0; i < NUM_SLAVES; ++i) {
        for (int j = 0; j < NUM_SCREENS; ++j) {
            for (int k = 0; k < SCREEN_DATA_SIZE; ++k) {
                uint8_t c = (data[i][j][k].empty() ? ' ' : std::stoi(data[i][j][k], nullptr, 16));
                frame.push_back(c);
            }
        }
    }

    frame.push_back(0x00);// 结束位
    frame.push_back(calculate_checksum(frame, 2, FRAME1_SIZE - 1));// 计算校验和 
    return frame;
}

// 构造第二帧数据 -- 按键灯光数据
std::vector<unsigned char> DataFrame::construct_frame2(unsigned char frame_header,
                                                       unsigned char device_id, int pgm,int pvw,
                                                       std::string transition_type, std::vector<int> dsk)
{
    std::vector<unsigned char> frame(FRAME2_SIZE, 0); // 初始化数据帧
    // std::vector<unsigned char> frame;
    // frame.clear();
    // frame.reserve(FRAME2_SIZE); // 预分配内存以提高效率
    size_t offset = 0;
    frame[offset++] = frame_header; // 帧头
    frame[offset++] = device_id;    // 设备ID

    unsigned char main_ctrl_keys_bytes[MAIN_CTRL_KEYS_SIZE] = {0};

    for (int i = 0; i < MAIN_CTRL_KEYS_SIZE; i++) main_ctrl_keys_bytes[i] = 1;
    if (transition_type == "mix") main_ctrl_keys_bytes[10] = 2;
    if (!dsk.empty()) { 
      for (size_t i = 0; i < dsk.size(); i++) {
        main_ctrl_keys_bytes[17 + i] = dsk[i];
      }
    }
    std::memcpy(frame.data() + offset, main_ctrl_keys_bytes,
                MAIN_CTRL_KEYS_SIZE);
    offset += MAIN_CTRL_KEYS_SIZE;

    unsigned char slave_keys[NUM_SLAVE_KEYS][SLAVE_KEYS_SIZE] = {{0}};
    for (int i = 0; i < NUM_SLAVE_KEYS; i++) {
      for (int j = 0; j < SLAVE_KEYS_SIZE; j++) {
        slave_keys[i][j] = 1;
      }
    }

    if(pgm > 0 && pgm < 13){
        slave_keys[3][pgm - 1] = 2;
    } else if(pgm > 12 && pgm < 25) {
        slave_keys[4][pgm - 12 - 1] = 2;
    } else {
        slave_keys[5][pgm - 24 - 1] = 2;
    }
    if(pvw > 0 && pvw < 13){
        slave_keys[3][12 + pvw - 1] = 3;
    } else if(pvw > 12 && pvw < 25) {
        slave_keys[4][12 + pvw - 12 - 1] = 3;
    } else {
        slave_keys[5][12 + pvw - 24 - 1] = 3;
    }

    for (int i = 0; i < NUM_SLAVES; i++)
    {
      std::memcpy(frame.data() + offset, slave_keys[i], SLAVE_KEYS_SIZE);
      offset += SLAVE_KEYS_SIZE;
    }
    frame[offset++] = 0x00; // 结束位
    // 添加帧尾部（1字节校验和）
    frame[offset] = calculate_checksum(frame, 2, FRAME2_SIZE - 1);
    return frame;
}

// 计算数据的校验和
unsigned char DataFrame::calculate_checksum(const std::vector<unsigned char> &data, size_t start, size_t end)
{
    unsigned char sum = 0;
    for (size_t i = start; i < end; ++i)
    {
      sum += data[i];
    }
    return sum;
}


// 主控制类 -- 处理服务器的消息并发送数据帧
MainController::MainController(const UARTConfig &uart_config1, const UARTConfig &uart_config3)
    : uart1_(uart_config1), uart3_(uart_config3), data_frame_(){}

MainController::~MainController() {
    stop();
}

void MainController::init() {
    uart1_.init();
    uart3_.init();
    // canSocket_.init();
}
void MainController::start() {
    uart1_.start();
    uart3_.start();
    // canSocket_.start();
}
void MainController::stop() {
    uart1_.stop();
    uart3_.stop();
    // canSocket_.stop();
}

//发送数据帧
void MainController::sendFrames()
{
    auto lock = acquire_lock();
    try{
        // std::vector<unsigned char> frame1 = data_frame_.construct_frame1(0xA8,0x01);
        uart1_.sendFrame(screen_frame_data);
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(2000));
        lock.lock();
        std::vector<unsigned char> frame2 = data_frame_.construct_frame2(0xB7, 0x01, this->pgm, this->pvw, this->transition_type, this->dsk);
        uart3_.sendFrame(frame2);
    }
    catch (const std::exception &e){
        logger->error("发送错误: {}", e.what());
    }
    #ifdef DEBUG_SEND_LOG
        format_frame1(frame1);
        format_frame2(frame2);
    #endif
}


// 打印数据帧1 -- 屏幕数据
std::vector<unsigned char> MainController::format_frame1(const std::vector<unsigned char> &data)
{
    std::cout << "Frame1 data:" << std::endl;
    for (size_t i = 0; i < data.size(); ++i) {
        std::cout << "0x" << std::hex << std::uppercase <<std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
        if ((i + 1) % 40 == 0) {
            std::cout  << std::endl;
        }
    }
    return data;
}

// 打印数据帧2 -- 按键灯光数据
std::vector<unsigned char> MainController::format_frame2(const std::vector<unsigned char> &data)
{
    std::cout << "Frame2 data:" << std::endl;
    for (size_t i = 0; i < data.size(); ++i) {
        std::cout  << std::hex << std::uppercase << static_cast<int>(data[i]) << " ";
        if ((i + 1) % 16 == 0) {
            std::cout  << std::endl;
        }
    }
    return data;
}

// 接收服务器的消息，构造并发送数据帧 -- DSK
void MainController::handler_dsk_status(std::vector<bool> dsk_status)
{
    bool sendFlag = false;
    auto lock = acquire_lock();

    for (size_t i = 0; i < this->dsk.size(); i++){
      if (i < dsk_status.size()){
        int status = dsk_status[i] ? 2 : 1;
        if (status != this->dsk[i]){
          sendFlag = true;
          this->dsk[i] = status;
        }
      }
    }
    if (sendFlag){
        lock.unlock();  
        sendFrames();
    }
}

// 接收服务器的消息，构造并发送数据帧 -- nextkey
void MainController::handler_nextkey(std::vector<bool> nextkey_status)
{
  // 处理下一个键值   
  bool sendFlag = false;
  for (size_t i = 0; i < this->nextkey.size(); i++){
      if (i < nextkey_status.size()){
        int status = nextkey_status[i] ? 1 : 0;
        if (status != this->nextkey[i]){
          sendFlag = true;
          this->nextkey[i] = status;
        }
      }
    }
    for (int i = 0; i < nextkey_status.size(); i++){
        logger->info("nextkey[{}]: {}", i, nextkey_status[i] ? "true" : "false");
    }
    if (sendFlag){
        sendFrames();
    }

}

// 接收服务器的消息，构造并发送数据帧 -- mode
void MainController::handler_mode(std::vector<bool> mode)
{
  sendFrames();
}

// 接收服务器的消息，构造并发送数据帧 -- bkgd
void MainController::handler_bkgd(std::vector<bool> bkgd)
{
  sendFrames();
}

// 接收服务器的消息，构造并发送数据帧 -- prevtrans
void MainController::handler_prevtrans(std::vector<bool> prevtrans)
{
  sendFrames();
}

//接收服务器的消息，构造并发送数据帧 -- shift
void MainController::handler_shift(std::vector<bool> shift)
{
  sendFrames();
}


// 接收服务器的消息，构造并发送数据帧 -- MIX
void MainController::handler_transition_type(std::string transition_type)
{
    auto lock = acquire_lock();
    if (this->transition_type != transition_type){
      this->transition_type = transition_type;
      lock.unlock();
      sendFrames();
    }
}

// 接收服务器的消息，构造并发送数据帧 -- PGM PVW
void MainController::handler_status(int pgm_, int pvw_)
{
    auto lock = acquire_lock();
    pgm_ += 1; 
    pvw_ += 1; 
    if (pgm_ == this->pgm && pvw_ == this->pvw){
      return;
    }
    if (pgm_ != this->pgm){
      this->pgm = pgm_;
    }
    if (pvw_ != this->pvw){
      this->pvw = pvw_;
    }
    lock.unlock();
    sendFrames();
}

void MainController::handle_key_press(const std::string &key) {
    
    if (key == "mix") {
        nlohmann::json trans_type;
        {
            auto lock = acquire_lock();
            if (transition_type.empty()) {
                trans_type = "mix";
            } else {
                trans_type = nlohmann::json();
            }
        }
        switcher->mix(trans_type);
    }
}


};//namespace seeder