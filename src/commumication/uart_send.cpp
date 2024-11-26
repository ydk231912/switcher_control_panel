#include "uart_send.h"
#include <cstring>

namespace seeder {

UART::UART(const UARTConfig &config) : port_(config.port), baud_rate_(config.baud_rate), fd_(-1){   
    openPort();  // 打开串口
}

UART::~UART() {
    stop();
}

void UART::openPort() {
    fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ == -1){
        logger->error("Failed to open the serial port: {}, uart reconnecting", port_);
        reconnect();
        return;
    } else {
        logger->info("Serial port opened successfully: {}", port_);
    }
    configure(); // 配置串口参数
}

void UART::closePort() {
    if (fd_ != -1){
        close(fd_);
        fd_ = -1;
        logger->info("Serial port closed: {}", port_);
    }
}

// 配置串口参数
void UART::configure() {
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd_, &tty) != 0){
        logger->error("tcgetattr() error: {}", strerror(errno));
        reconnect();
        return;
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
        reconnect();
        return;
    } else {
        logger->info("Serial port configured successfully: {}", port_);
    }
}

bool UART::checklinkstatus() {
    if (fd_ == -1) return false;
    if (fcntl(fd_, F_GETFL) == -1) {
        logger->error("UART connection lost: {}", strerror(errno));
        return false;
    }
    return true;
}
void UART::reconnect() {
    logger->info("Attempting to reconnect to UART port: {}", port_);
    while (!checklinkstatus()) {
        closePort();
        openPort();
        usleep(10000);
    }
    logger->info("Successs to reconnect to UART port: {}", port_);
}

void UART::sendFrame(const std::vector<unsigned char> &data)
{
    if (!checklinkstatus()) {
        logger->error("UART port is not open");
        reconnect();
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
}

void UART::start() {}

void UART::stop() {
    closePort();
}

DataFrame::DataFrame(const std::shared_ptr<config::Config> control_panel_config, std::shared_ptr<Switcher> switcher) : 
    control_panel_config(control_panel_config), switcher(switcher) {
    init();
}

DataFrame::~DataFrame() {}

void DataFrame::init() {
    num_slave = control_panel_config->panel_config.num_slave;
    num_screen = control_panel_config->panel_config.num_screen;
    screen_data_size = control_panel_config->panel_config.screen_data_size;
    oled_frame_size = control_panel_config->panel_config.oled_frame_size;
    key_each_row_num = control_panel_config->panel_config.key_each_row_num;
    switcher_keyboard_id = control_panel_config->panel_config.switcher_keyboard_id;
    key_difference_value = control_panel_config->panel_config.key_difference_value;
    main_ctrl_keys_size = control_panel_config->panel_config.main_ctrl_keys_size;
    slave_keys_size = control_panel_config->panel_config.slave_keys_size;
    key_frame_size = control_panel_config->panel_config.key_frame_size;
    slave_key_each_row_num = control_panel_config->panel_config.slave_keys_size;
}

// 函数：从文件中读取十六进制数据并解析为 2x3x40 的三维数组
std::vector<std::vector<std::vector<std::string>>> DataFrame::readHexData(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::in);  // 打开文件进行读取
    // 初始化 2x3x40 的三维数组
    std::vector<std::vector<std::vector<std::string>>> data(control_panel_config->panel_config.num_slave, std::vector<std::vector<std::string>>(control_panel_config->panel_config.num_screen, std::vector<std::string>(control_panel_config->panel_config.screen_data_size, ""))); 
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
                if (layerIndex < control_panel_config->panel_config.num_slave && rowIndex < control_panel_config->panel_config.num_screen && colIndex < control_panel_config->panel_config.screen_data_size) {
                    data[layerIndex][rowIndex][colIndex] = hexStr;
                    colIndex++;
                    // 当一列填满后，移动到下一行
                    if (colIndex >= control_panel_config->panel_config.screen_data_size) {
                        colIndex = 0;
                        rowIndex++;
                        // 当一行填满后，移动到下一层
                        if (rowIndex >= control_panel_config->panel_config.num_screen) {
                            rowIndex = 0;
                            layerIndex++;
                        }
                    }
                }
            }
        }
        // 如果已填满整个三维数组，则退出循环
        if (layerIndex >= control_panel_config->panel_config.num_slave) {
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
    for (int layer = 0; layer < control_panel_config->panel_config.num_slave; ++layer) {
        for (int row = 0; row < control_panel_config->panel_config.num_screen; ++row) {
            for (int col = 0; col < control_panel_config->panel_config.screen_data_size; ++col) {
                ofs << data[layer][row][col] << ' ';
            }
            ofs << std::endl;  // 每行数据后换行
        }
    }
    logger->info("write file");

    ofs.close();  // 关闭文件
}

// 构造第一帧数据 -- OLED显示屏数据
std::vector<unsigned char> DataFrame::construct_frame1(
    unsigned char frame_header,
    unsigned char device_id,
    const std::vector<std::vector<std::vector<std::string>>> &data)
{
    // std::vector<unsigned char> frame(control_panel_config->panel_config.oled_frame_size, 0); // 初始化数据帧
    std::vector<unsigned char> frame;
    frame.reserve(control_panel_config->panel_config.oled_frame_size); // 预分配内存以提高效率
    frame.push_back(frame_header);// 帧头
    frame.push_back(device_id);// 设备ID
    // 将三维数组的数据转换为 unsigned char 并存储到 vector 中
    for (int i = 0; i < control_panel_config->panel_config.num_slave; ++i) {
        for (int j = 0; j < control_panel_config->panel_config.num_screen; ++j) {
            for (int k = 0; k < control_panel_config->panel_config.screen_data_size; ++k) {
                uint8_t c = (data[i][j][k].empty() ? ' ' : std::stoi(data[i][j][k], nullptr, 16));
                frame.push_back(c);
            }
        }
    }

    frame.push_back(0x00);// 结束位
    frame.push_back(calculate_checksum(frame, 2, control_panel_config->panel_config.oled_frame_size - 1));// 计算校验和 
    return frame;
}

// 构造第二帧数据 -- 按键灯光数据
std::vector<unsigned char> DataFrame::construct_frame2(unsigned char frame_header, unsigned char device_id, 
                                                       int pgm, int pvw, std::string transition_type, std::vector<int> dsk, int proxy_key)
{
    size_t offset = 0;
    std::vector<unsigned char> frame(key_frame_size, 0); // 初始化数据帧
    
    frame[offset++] = frame_header; // 帧头
    frame[offset++] = device_id;    // 设备ID
    
    std::vector<unsigned char> main_ctrl_keys(main_ctrl_keys_size, 1);
    if (transition_type == "mix") {
        main_ctrl_keys[10] = key_colour[5];
    } else if (transition_type == "dip") {
        main_ctrl_keys[11] = key_colour[5];
    } else if (transition_type == "luma_wipe") {
        main_ctrl_keys[12] = key_colour[5];
    } else if (transition_type == "slide") {
        main_ctrl_keys[13] = key_colour[5];
    } else if (transition_type == "dve") {
        main_ctrl_keys[14] = key_colour[5];
    } else {

    }
    if (!dsk.empty()) {
        for (size_t i = 0; i < dsk.size() && i < main_ctrl_keys_size - 17; ++i) {
            main_ctrl_keys[17 + i] = dsk[i];
        }
    }
    std::memcpy(frame.data() + offset, main_ctrl_keys.data(), main_ctrl_keys_size);
    offset += main_ctrl_keys_size;

    std::vector<std::vector<unsigned char>> slave_keys(num_slave, std::vector<unsigned char>(slave_keys_size, 1));

    auto update_slave_key = [&](int index, int value, unsigned char status) {
        if (value > 0 && value <= slave_keys_size) {
            slave_keys[index][value - 1] = status;
        } else {
            std::cerr << "Invalid index or value: index=" << index << ", value=" << value << std::endl;
        }
    };
    if (switcher->pgm_shift_status != switcher->sw_pgm_pvw_shift_status[0]) {
        // logger->info("The status of the PGM shift key is incorrect");
    } else {
        if (pgm > 0) {
            if (switcher_keyboard_id == 1) {
                update_slave_key(1, pgm, key_colour[2]);
            } else {
                update_slave_key(3, (pgm <= slave_key_each_row_num) ? pgm : 0, key_colour[2]);
                update_slave_key(4, (pgm > slave_key_each_row_num && pgm <= slave_key_each_row_num*2) ? pgm - slave_key_each_row_num : 0, key_colour[2]);
                update_slave_key(5, (pgm > slave_key_each_row_num*2) ? pgm - slave_key_each_row_num*2 : 0, key_colour[2]);
            }
        }
    };
    if (switcher->pvw_shift_status != switcher->sw_pgm_pvw_shift_status[1]) {
        // logger->info("The status of the PVW shift key is incorrect");
    } else {
        if (pvw > 0) {
            if (switcher_keyboard_id == 1) {
                update_slave_key(1, key_difference_value + pvw, key_colour[3]);
            } else {
                update_slave_key(3, (pvw <= slave_key_each_row_num) ? key_difference_value + pvw : 0, key_colour[3]);
                update_slave_key(4, (pvw > slave_key_each_row_num && pvw <= slave_key_each_row_num*2) ? pvw + key_difference_value - slave_key_each_row_num : 0, key_colour[3]);
                update_slave_key(5, (pvw > slave_key_each_row_num*2) ? pvw + key_difference_value - slave_key_each_row_num*2 : 0, key_colour[3]);
            }
        }
    }
    
    for (const auto& keys : slave_keys) {
        std::memcpy(frame.data() + offset, keys.data(), slave_keys_size);
        offset += slave_keys_size;
    }

    if (switcher_keyboard_id == 2) {
        frame[offset++] = 0x00;
    }
    frame[offset] = calculate_checksum(frame, 2, key_frame_size - 1);

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
MainController::MainController(const UARTConfig &uart_config1, const UARTConfig &uart_config2, 
                        const std::shared_ptr<config::Config> control_panel_config, 
                        const std::shared_ptr<DataFrame> data_frame, std::shared_ptr<Switcher> switcher)
    : uart1_(uart_config1), uart3_(uart_config2), control_panel_config(control_panel_config), 
        data_frame_(data_frame), switcher(switcher) {
    init();
}

MainController::~MainController() {
    stop();
}

void MainController::init() {
    key_each_row_num = control_panel_config->panel_config.key_each_row_num;
    max_source_num = control_panel_config->panel_config.max_source_num;
}
void MainController::start() {
    uart1_.start();
    uart3_.start();
}
void MainController::stop() {
    uart1_.stop();
    uart3_.stop();
}

//发送数据帧
void MainController::sendFrames()
{
    auto lock = acquire_lock();
    try{
        // std::vector<unsigned char> frame1 = data_frame_.construct_frame1(0xA8,0x01);
        uart1_.sendFrame(screen_frame_data);
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        lock.lock();
        // std::vector<unsigned char> frame2 = data_frame_.construct_frame2(0xB7, 0x01, this->pgm, this->pvw, this->transition_type, this->dsk);
        key_status_data = data_frame_->construct_frame2(0xB7, 0x01, this->pgm, this->pvw, this->transition_type, this->dsk, this->proxy_key);
        uart3_.sendFrame(key_status_data);
    }
    catch (const std::exception &e){
        logger->error("发送错误: {}", e.what());
    }
    lock.unlock();
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
    lock.unlock();
    if (sendFlag){
        sendFrames();
    }
    
}

// 接收服务器的消息，构造并发送数据帧 -- nextkey
void MainController::handler_nextkey(std::vector<bool> nextkey_status)
{
  // 处理下一个键值   
  auto lock = acquire_lock();
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
    lock.unlock();
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
    int i_source = key_each_row_num - 1;
    pgm_ += 1; 
    pvw_ += 1;

    auto lock = acquire_lock();

    int pgm_offset = (pgm_ - 1) / i_source;
    int pvw_offset = (pvw_ - 1) / i_source;
    lock.unlock();
    if ((pgm_offset <= max_source_num) && (pvw_offset <= max_source_num)) {
        if (this->pgm == pgm_ && this->pvw == pvw_) {
            return;
        } else {
            switcher->sw_pgm_pvw_shift_status = {pgm_offset, pvw_offset};
            this->pgm = pgm_ - pgm_offset * i_source;
            this->pvw = pvw_ - pvw_offset * i_source;
            sendFrames();
        }
    } else {
        logger->warn("The number of input sources exceeds the configured quantity");
    }
}

// 代理键
void MainController::handle_proxy_keys_status(
    std::vector<bool>is_on_air0, std::vector<bool>is_tied0, int fill_source_index, 
    std::vector<bool>is_on_air1, std::vector<bool>is_tied1, int key_source_index){
    
}

void MainController::handle_transition_press(const std::string &transition) 
{
    auto lock = acquire_lock();
    nlohmann::json trans_type = nlohmann::json();
    {
        if (transition == "mix") {
            trans_type = "mix";
        } else if (transition == "dip") {
            trans_type = "dip";
        } else if (transition == "luma_wipe") {
            trans_type = "luma_wipe";
        } else if (transition == "slide") {
            trans_type = "slide";
        } else if (transition == "dve") {
            trans_type = "dve";
        } else {
            logger->warn("The transition effect incoming type {}, is incorrect",transition);
        }
        lock.unlock();
    }
    lock.lock();
    if (old_trans_type != trans_type) {
        switcher->set_transition(trans_type);
    } else {
        trans_type = nlohmann::json();
        switcher->set_transition(trans_type);
    }
    old_trans_type = trans_type;
    lock.unlock();
}

void MainController::handle_proxy_press(const int proxy_idx, const std::string &proxy_type) {
    if (!proxy_type.empty()) {
        if (proxy_type == "key") {
            
        }

    } else {
        logger->warn("The downstream type incoming type {}, is incorrect", proxy_type);
    }
}

void MainController::handle_proxy_sources_press(std::vector<int> proxy_sources) {
    // int i_source = key_each_row_num - 1;
    // pgm_ += 1; 
    // pvw_ += 1;

    // auto lock = acquire_lock();

    // int pgm_offset = (pgm_ - 1) / i_source;
    // int pvw_offset = (pvw_ - 1) / i_source;
    // lock.unlock();
    // if ((pgm_offset <= max_source_num) && (pvw_offset <= max_source_num)) {
    //     if (this->pgm == pgm_ && this->pvw == pvw_) {
    //         return;
    //     } else {
    //         switcher->sw_pgm_pvw_shift_status = {pgm_offset, pvw_offset};
    //         this->pgm = pgm_ - pgm_offset * i_source;
    //         this->pvw = pvw_ - pvw_offset * i_source;
    //         sendFrames();
    //     }
    // } else {
    //     logger->warn("The number of input sources exceeds the configured quantity");
    // }
}

void MainController::handle_get_key_status(nlohmann::json param) {
    
}

};//namespace seeder