#include "uart_send.h"

namespace seeder {

UART::UART(const UARTConfig &config)
    : port_(config.port), baud_rate_(config.baud_rate), fd_(-1)
{
    logger->info("Initializing UART on port: {}", port_);
    openPort();  // 打开串口
    configure(); // 配置串口参数
}

UART::~UART()
{
    logger->info("Closing UART port: {}", port_);
    closePort(); // 析构时关闭串口
}

// 发送数据帧
void UART::sendFrame(const std::vector<unsigned char> &data)
{
    if (fd_ == -1)
    {
        logger->error("UART port is not open");
        throw std::runtime_error("UART端口未打开");
    }
    tcflush(fd_, TCIOFLUSH);                              // 清空UART缓冲区
    ssize_t bytes_written = write(fd_, data.data(), data.size()); // 写入数据
    if (bytes_written < 0)
    {
        logger->error("Failed to write data to UART: {}", strerror(errno));
        throw std::runtime_error("写入数据失败");
    }
    else if (static_cast<size_t>(bytes_written) != data.size())
    {
        logger->warn("Sent data length does not match expected length");
    }
}

// 打开串口
void UART::openPort()
{
    fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ == -1)
    {
        logger->error("Unable to open UART port: {}", port_);
        throw std::runtime_error("无法打开UART端口: " + port_);
    }
}

// 关闭串口
void UART::closePort()
{
    if (fd_ != -1)
    {
        close(fd_);
        fd_ = -1;
    }
}

// 配置串口参数
void UART::configure()
{
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd_, &tty) != 0){
        logger->error("tcgetattr() error: {}", strerror(errno));
        throw std::runtime_error("tcgetattr() 错误");
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
        throw std::runtime_error("tcsetattr() 错误");
    }
}


// 函数：从文件中读取十六进制数据并解析为 2x3x40 的三维数组
std::vector<std::vector<std::vector<std::string>>> DataFrame::readHexData(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::in);  // 打开文件进行读取
    // 初始化 2x3x40 的三维数组
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
    return data;  // 返回读取的 2x3x40 三维数组
}

// 函数：将三维数组的数据写回到文件中
void DataFrame::writeHexData(const std::string& filename, const std::vector<std::vector<std::vector<std::string>>>& data) {
    std::ofstream ofs(filename, std::ios::out);  // 打开文件进行写入
    if (!ofs.is_open()) {
        logger->error("Error opening file for writing: {}", filename);
        return;
    }

    // 将三维数组中的数据写回到文件
    for (int layer = 0; layer < NUM_SLAVES; ++layer) {
        for (int row = 0; row < NUM_SCREENS; ++row) {
            for (int col = 0; col < SCREEN_DATA_SIZE; ++col) {
                ofs << data[layer][row][col] << ' ';
            }
            ofs << std::endl;  // 每行数据后换行
        }
    }
    // for (const auto& layer : data){
    //     for (const auto& row : layer){
    //         for (const auto& col : row){
    //             ofs << col << ' ';
    //         }
    //         ofs << '\n';
    //     }
    // }

    ofs.close();  // 关闭文件
}

//对OLED显示数据文件进行操作
std::vector<std::vector<std::vector<std::string>>> DataFrame::file_event_handlers() {
    const std::string filename = "../data.txt";  // 文件路径（请根据实际文件路径修改）

    // 从文件中读取十六进制数据并解析为 2x3x40 三维数组
    std::vector<std::vector<std::vector<std::string>>> data = readHexData(filename);

    // 修改OLED显示屏数据
    if (data[0][0].size() > 4) {  // 确保数组足够大
        data[0][0][4] = "0xAB";  // 修改为新的十六进制值，例如 "0xAB"
    }

    // 将修改后的数据写回到文件中
    writeHexData(filename, data);

    return data;
}

// 构造第一帧数据 -- OLED显示屏数据
std::vector<unsigned char> DataFrame::construct_frame1(unsigned char frame_header,
                                                       unsigned char device_id)
{
    std::vector<unsigned char> frame;
    frame.reserve(FRAME1_SIZE); // 预分配内存以提高效率
    std::vector<std::vector<std::vector<std::string>>> data = file_event_handlers();
    frame.push_back(frame_header);// 帧头
    frame.push_back(device_id);// 设备ID
    // 将三维数组的数据转换为 unsigned char 并存储到 vector 中
    for (int i = 0; i < NUM_SLAVES; ++i) {
        for (int j = 0; j < NUM_SCREENS; ++j) {
            for (int k = 0; k < SCREEN_DATA_SIZE; ++k) {
                frame.push_back(std::stoi(data[i][j][k], nullptr, 16));
            }
        }
    }
    // for (const auto& layer : data){
    //     for (const auto& row : layer){
    //         for (const auto& col : row){
    //             frame.push_back(static_cast<unsigned char>(std::stoi(col, nullptr, 16)));
    //         }
    //     }
    // }

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
    slave_keys[1][pgm - 1] = 2;
    slave_keys[1][12 + pvw - 1] = 3;
    for (int i = 0; i < 2; i++)
    {
      std::memcpy(frame.data() + offset, slave_keys[i], SLAVE_KEYS_SIZE);
      offset += SLAVE_KEYS_SIZE;
    }
    // frame[offset] = 0x00; // 结束位
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
    : uart1_(uart_config1), uart3_(uart_config3), stop_flag_(false)
{
    logger->info("MainController initialized");
    sender_thread_ = std::thread(&MainController::sendFrames, this);
}

//发送数据帧
void MainController::sendFrames()
{
    if(!stop_flag_){
        try{
            std::vector<unsigned char> frame1 = data_frame_.construct_frame1(0xA8,0x01);
            uart1_.sendFrame(frame1);
            std::this_thread::sleep_for(std::chrono::microseconds(2000));
            std::vector<unsigned char> frame2 = data_frame_.construct_frame2(0xB7, 0x01, this->pgm, this->pvw, this->transition_type, this->dsk);
            uart3_.sendFrame(frame2);
            
            logger->info("Frame 1 and Frame 2 sent successfully");

            logger->info("Frame 1 data:\n{}");
            format_frame1(frame1);
            logger->info("Frame 2 data:\n{}");
            format_frame2(frame2);
        }
        catch (const std::exception &e){
        logger->error("发送错误: {}", e.what());
        }
    } else {
        logger->info("Stop flag is set, skipping frame send");
        return;
    }
}

// 停止发送数据帧
void MainController::stop()
{
    stop_flag_ = true; // 设置停止标志
    if (sender_thread_.joinable()){
        sender_thread_.join(); // 等待发送线程结束
    }
    if (printer_thread_.joinable()){
      printer_thread_.join(); // 等待打印线程结束
    }
}

// 打印数据帧1 -- 屏幕数据
std::vector<unsigned char> MainController::format_frame1(const std::vector<unsigned char> &data)
{
    std::cout << "Frame1 data:" << std::endl;
    for (size_t i = 0; i < data.size(); ++i) {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
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
    stop_flag_ = false;

    for (size_t i = 0; i < this->dsk.size(); i++){
      if (i < dsk_status.size()){
        int status = dsk_status[i] ? 2 : 1;
        if (status != this->dsk[i]){
          stop_flag_ = true;
          this->dsk[i] = status;
        }
      }
    }
    for (int i = 0; i < dsk_status.size(); i++){
        logger->info("dsk[{}]: {}", i, dsk_status[i] ? "true" : "false");
    }
    if (stop_flag_){
      sendFrames();
    }
}

// 接收服务器的消息，构造并发送数据帧 -- nextkey
void MainController::handler_nextkey(std::vector<bool> nextkey_status)
{
  // 处理下一个键值   
  stop_flag_ = false;
  for (size_t i = 0; i < this->nextkey.size(); i++){
      if (i < nextkey_status.size()){
        int status = nextkey_status[i] ? 1 : 0;
        if (status != this->nextkey[i]){
          stop_flag_ = true;
          this->nextkey[i] = status;
        }
      }
    }
    for (int i = 0; i < nextkey_status.size(); i++){
        logger->info("nextkey[{}]: {}", i, nextkey_status[i] ? "true" : "false");
    }
    if (stop_flag_){
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
    if (this->transition_type != transition_type){
      this->transition_type = transition_type;
      sendFrames();
    }
}

// 接收服务器的消息，构造并发送数据帧 -- PGM PVW
void MainController::handler_status(int pgm_, int pvw_)
{
    stop_flag_ = false;
    
    pgm_ += 1; 
    pvw_ += 1; 
    logger->info("receive pgm : {}, pvw : {}", pgm_, pvw_);
    if (pgm_ == this->pgm && pvw_ == this->pvw){
      stop_flag_ = true;
    }
    if (pgm_ != this->pgm){
      this->pgm = pgm_;
    }
    if (pvw_ != this->pvw){
      this->pvw = pvw_;
    }
    sendFrames();
}

};//namespace seeder