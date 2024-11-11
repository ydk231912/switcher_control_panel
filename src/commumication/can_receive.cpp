#include "can_receive.h"

namespace seeder {

CanSocket::CanSocket(const std::string &interface) : interface_name(interface) { setupSocket(); } //初始化 CAN 套接字

CanSocket::~CanSocket() {close(socket_fd);} // 关闭套接字

// 设置并初始化 CAN 套接字
void CanSocket::setupSocket()
{
    struct sockaddr_can addr = {};
    struct ifreq ifr;

    // 创建 raw CAN 套接字
    if ((socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        logger->error("Socket creation failed", strerror(errno));
        reconnect();
        return;
    }

    // 初始化 sockaddr_can 结构体
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(interface_name.c_str());

    if (addr.can_ifindex == 0)
    {
        logger->error("Invalid CAN interface", strerror(errno));
        reconnect();
        return;
    }

    // 绑定套接字到 CAN 接口
    if (bind(socket_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        logger->error("Bind failed", strerror(errno));
        reconnect();
        return;
    }

    // 设置套接字为非阻塞模式
    setNonblocking();

    // 检查链接状态
    if (!checkLinkStatus()) {
        logger->error("CAN interface is down");
        reconnect();
        return;
    }
}

// 设置套接字为非阻塞模式
void CanSocket::setNonblocking()
{
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0)
    {
        logger->error("fcntl F_GETFL failed", strerror(errno));
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        logger->error("fcntl F_SETFL failed", strerror(errno));
    }
}

bool CanSocket::checkLinkStatus()
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ);
    if(socket_fd < 0) {
        return false;
    }
    if (ioctl(socket_fd, SIOCGIFFLAGS, &ifr) < 0) {
        logger->error("ioctl SIOCGIFFLAGS failed", strerror(errno));
        return false;
    }

    return (ifr.ifr_flags & IFF_RUNNING) != 0;
}

void CanSocket::reconnect()
{
    logger->error("Attempting to reconnect to UART port: {}", interface_name.c_str());
    while (!checkLinkStatus()) {
        close(socket_fd);
        setupSocket();
        usleep(10000);
    }
    logger->error("Successs to reconnect to UART port: {}", interface_name.c_str());
}

// 获取套接字文件描述符
int CanSocket::getSocketFd() const {
    return socket_fd;
} 

void CanSocket::init() { setupSocket(); }
void CanSocket::start() {}
void CanSocket::stop() {close(socket_fd);}



//接受 CanSocket 实例
CanFrameProcessor::CanFrameProcessor(CanSocket &canSocket, std::shared_ptr<seeder::Switcher> sw)
    : socket(canSocket), switcher(sw){
    init();
}

CanFrameProcessor::~CanFrameProcessor() {
  stop();
}

void CanFrameProcessor::init() {
    switcher_keyboard_id = switcher->control_panel_config->panel_config.switcher_keyboard_id;
    can_frame_ids = switcher->control_panel_config->panel_config.can_frame_ids;
    key_each_row_num = switcher->control_panel_config->panel_config.key_each_row_num;
    key_difference_value = switcher->control_panel_config->panel_config.key_difference_value;
    // key_source_sum = switcher->key_source_sum;
    // key_source_shift_status = switcher->key_source_shift_status;

    for (int i = 0; i < 4; ++i) {
        main_key_press_states[i].init(main_Keys[i], 0);
    }
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 3; ++j) {
            slave_key_press_states[i][j].init(slave_Keys[i][j], 0);
        }
    }
    clipper_state.init("clipper_manual_switching", 0);
}

// 启动 CAN 帧处理线程
void CanFrameProcessor::start()
{
    processing_thread = std::thread(&CanFrameProcessor::processFrames, this);
}

// 停止 CAN 帧处理线程
void CanFrameProcessor::stop()
{
    processing_thread.join();
}

// 处理 CAN 帧数据
void CanFrameProcessor::processFrames()
{
    can_frame frame;
    int nbytes = 0;
    while (1)
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
            logger->error("CAN read failed", strerror(errno));
        }
    }
}

// 处理 CAN 帧数据
void CanFrameProcessor::processFrame(const can_frame &frame)
{
    if(switcher_keyboard_id == 1) {
        if(frame.can_id == can_frame_ids[0]){
            handle_FrameData(frame);
        } else if(frame.can_id == can_frame_ids[1]){
            // handle_FrameData1(frame);
        } else if(frame.can_id == can_frame_ids[2]){
            handle_FrameData4(frame);
        } else {
            logger->error("frame can id error, frame_can_id = {}",frame.can_id);
        }
    }else if((switcher_keyboard_id == 2)){
        if(frame.can_id == can_frame_ids[0]){
            handle_FrameData(frame);
        } else if(frame.can_id == can_frame_ids[1]){
            handle_FrameData1(frame);
        } else if(frame.can_id == can_frame_ids[2]){
            handle_FrameData2(frame);
        } else if(frame.can_id == can_frame_ids[3]){
            handle_FrameData3(frame);
        } else if(frame.can_id == can_frame_ids[4]){
            handle_FrameData4(frame);
        } else if(frame.can_id == can_frame_ids[5]){
            handle_FrameData5(frame);
        } else if(frame.can_id == can_frame_ids[6]){
            handle_FrameData6(frame);
        } else {
            logger->error("frame can id error, frame_can_id = {}",frame.can_id);
        }
    }
}

// 处理并发送 CAN 帧数据 -- 推子板
void CanFrameProcessor::handle_FrameData(const can_frame &frame)
{
    //key handle
    if (main_key_press_states[0].update(frame.data[0] != 0x00 ? 1 : 0) && main_key_press_states[0].value) {
        idx = calculateidex(frame.data[0], 1);
        handles_clipper_key(idx);
    }
    if (main_key_press_states[1].update(frame.data[1] != 0x00 ? 1 : 0) && main_key_press_states[1].value) {
        idx = calculateidex(frame.data[1], can_array_every + 1);
        handles_clipper_key(idx);
    }
    if (main_key_press_states[2].update(frame.data[2] != 0x00 ? 1 : 0) && main_key_press_states[2].value) {
        idx = calculateidex(frame.data[2], can_array_every*2 + 1);
        handles_clipper_key(idx);
    }
    if (main_key_press_states[3].update(frame.data[3] != 0x00 ? 1 : 0) && main_key_press_states[3].value) {
        idx = calculateidex(frame.data[3], can_array_every*3 + 1);
        handles_clipper_key(idx);
    }
    //当progress_ratio=0且is_playing为false时，用户在界面上推动滑块将开始执行手动切换
    this->progress_ratio_now = clipper_handle(frame);
    // std::cout << "progress_ratio_now : " << progress_ratio_now << std::endl;
    if (clipper_state.update(this->progress_ratio_now != this->progress_ratio ? 1 : 0) && clipper_state.value) {
        if (!this->is_auto) {
            if (this->progress_ratio_now == 0) {
                is_up_cut = true;
                can_start_clipper = true;
            }
            if (this->progress_ratio_now == clipper_max) {
                is_up_cut = false;
                can_start_clipper = true;
            }
            if (can_start_clipper) {
                if(this->progress_ratio_now != clipper_max && this->progress_ratio_now != 0) {
                    switcher->mixer_command_manual(this->is_auto);
                }
            }
        } else if (this->is_auto) {
            if (is_up_cut) {
                switcher->mixer_command_manual_ratio(this->progress_ratio_now);
            } else if (!is_up_cut) {
                switcher->mixer_command_manual_ratio(clipper_max - progress_ratio_now);
            }

            if (this->progress_ratio_now == 0) {
                is_up_cut = true;
                can_start_clipper = true;
            } else if (this->progress_ratio_now == clipper_max) {
                is_up_cut = false;
                can_start_clipper = true;
            } else {
                can_start_clipper = false;
            }
        }
        progress_ratio = this->progress_ratio_now;
        
    }
}

void CanFrameProcessor::handle_panel_status_update(nlohmann::json param) {
    // std::cout << param.dump(4) << std::endl;
    if (param["transition_status"]["is_playing"]) {
        if (!this->is_auto) {
            // switcher->mixer_command_manual_ratio(this->progress_ratio_now);
            this->is_auto = true;
        }
    } else if (!param["transition_status"]["is_playing"]) {
        this->is_auto = false;
    } else {
        return;
    }
}

// 处理并发送 CAN 帧数据 -- 从站1
void CanFrameProcessor::handle_FrameData1(const can_frame &frame)
{
    //mode -- 按一次切换到：KEY->AUX->UTL->MACRO->SNAPSHOT->M/E，长按可以对KEY、AUX、UTL、MACRO、SNAPSHOT、M/E进行选择
    //代理键类型
    if (slave_key_press_states[0][0].update(frame.data[0] != 0x00 ? 1 : 0) && slave_key_press_states[0][0].value) {
        idx = calculateidex(frame.data[0], 1);
        // handles_proxy_key(idx);
    }
    //代理源
    if (slave_key_press_states[0][1].update(frame.data[1] != 0x00 ? 1 : 0) && slave_key_press_states[0][1].value) {
        idx = calculateidex(frame.data[1], 9);
        if (idx != 12) {
            // handles_proxy_key(idx);
        }
    }
    if (slave_key_press_states[0][2].update(frame.data[2] != 0x00 ? 1 : 0) && slave_key_press_states[0][2].value) {
        idx = calculateidex(frame.data[2], 17);
        // handles_proxy_key(idx);
    }
    /*
    //处理mode按键长按状态
    if (frame.data[1] != 0x08) {
        return;
    } else if (frame.data[1] == 0x08) {
        this->mode_status == LongPress;
    }*/
    
}

// 处理并发送 CAN 帧数据 -- 从站2
void CanFrameProcessor::handle_FrameData2(const can_frame &frame)
{
    if (slave_key_press_states[1][0].update(frame.data[0] != 0x00 ? 1 : 0) && slave_key_press_states[1][0].value)
    {
        idx = calculateidex(frame.data[0], 1);
    }
    if (slave_key_press_states[1][1].update(frame.data[1] != 0x00 ? 1 : 0) && slave_key_press_states[1][1].value)
    {
        idx = calculateidex(frame.data[1], 9);
    }
    if (slave_key_press_states[1][2].update(frame.data[2] != 0x00 ? 1 : 0) && slave_key_press_states[1][2].value)
    {
        idx = calculateidex(frame.data[2], 17);
    }
}
// 处理并发送 CAN 帧数据 -- 从站3
void CanFrameProcessor::handle_FrameData3(const can_frame &frame)
{
    if (slave_key_press_states[2][0].update(frame.data[0] != 0x00 ? 1 : 0) && slave_key_press_states[2][0].value)
    {
        idx = calculateidex(frame.data[0], 1);
    }
    if (slave_key_press_states[2][1].update(frame.data[1] != 0x00 ? 1 : 0) && slave_key_press_states[2][1].value)
    {
        idx = calculateidex(frame.data[1], 9);
    }
    if (slave_key_press_states[2][2].update(frame.data[2] != 0x00 ? 1 : 0) && slave_key_press_states[2][2].value)
    {
        idx = calculateidex(frame.data[2], 17);
    }
}
// 处理并发送 CAN 帧数据 -- 从站4
void CanFrameProcessor::handle_FrameData4(const can_frame &frame)
{
    if (slave_key_press_states[3][0].update(frame.data[0] != 0x00 ? 1 : 0) && slave_key_press_states[3][0].value) {
        idx = calculateidex(frame.data[0], 1);
        handles_pgm_pvw_module(idx, true);
    }
    if (slave_key_press_states[3][1].update(frame.data[1] != 0x00 ? 1 : 0) && slave_key_press_states[3][1].value) {
        idx = calculateidex(frame.data[1], can_array_every + 1);
        if (idx < 13) {handles_pgm_pvw_module(idx, true);} 
        else {handles_pgm_pvw_module(idx, false);}
    }
    if (slave_key_press_states[3][2].update(frame.data[2] != 0x00 ? 1 : 0) && slave_key_press_states[3][2].value) {
        idx = calculateidex(frame.data[2], can_array_every*2 + 1);
        handles_pgm_pvw_module(idx, false);
    }
}

// 处理并发送 CAN 帧数据 -- 从站5
void CanFrameProcessor::handle_FrameData5(const can_frame &frame)
{
    if (slave_key_press_states[4][0].update(frame.data[0] != 0x00 ? 1 : 0) && slave_key_press_states[4][0].value)
    {
        idx = calculateidex(frame.data[0], key_difference_value + 1);
        handles_pgm_pvw_module(idx, true);
    }
    if (slave_key_press_states[4][1].update(frame.data[1] != 0x00 ? 1 : 0) && slave_key_press_states[4][1].value)
    {
        idx = calculateidex(frame.data[1], key_difference_value + can_array_every + 1);
        if (idx < 25) {handles_pgm_pvw_module(idx, true);} 
        else {handles_pgm_pvw_module(idx, false);}
    }
    if (slave_key_press_states[4][2].update(frame.data[2] != 0x00 ? 1 : 0) && slave_key_press_states[4][2].value)
    {
        idx = calculateidex(frame.data[2], key_difference_value + can_array_every*2 + 1);
        handles_pgm_pvw_module(idx, false);
    }
}

// 处理并发送 CAN 帧数据 -- 从站6
void CanFrameProcessor::handle_FrameData6(const can_frame &frame)
{
    if (slave_key_press_states[5][0].update(frame.data[0] != 0x00 ? 1 : 0) && slave_key_press_states[5][0].value)
    {
        idx = calculateidex(frame.data[0], key_difference_value*2 + 1);
        handles_pgm_pvw_module(idx, true);
    }
    if (slave_key_press_states[5][1].update(frame.data[1] != 0x00 ? 1 : 0) && slave_key_press_states[5][1].value)
    {
        idx = calculateidex(frame.data[1], key_difference_value*2 + can_array_every + 1);
        if (idx < 37) {handles_pgm_pvw_module(idx, true);} 
        else {handles_pgm_pvw_module(idx, false);}
    }
    if (slave_key_press_states[5][2].update(frame.data[2] != 0x00 ? 1 : 0) && slave_key_press_states[5][2].value)
    {
        idx = calculateidex(frame.data[2], key_difference_value*2 + can_array_every*2 + 1);
        handles_pgm_pvw_module(idx, false);
    }
}

uint16_t CanFrameProcessor::clipper_handle(const can_frame &frame){
    uint8_t highbyte = frame.data[4];
    uint8_t lowbyte = frame.data[5];

    uint16_t progress_ratio_now = clipper_max - convertToPercentage(highbyte, lowbyte);
    
    return progress_ratio_now;
}

uint16_t CanFrameProcessor::convertToPercentage(uint8_t highbyte, uint8_t lowbyte) {
    // 将16进制的两个字节合并成一个10进制的模拟量值
    uint16_t analogValue = (static_cast<uint16_t>(highbyte) << 8) | lowbyte;

    if (analogValue > clipper_simulate) {
        analogValue = clipper_simulate;
    }
    return (analogValue * clipper_max) / clipper_simulate;
}


void CanFrameProcessor::handles_pgm_pvw_module(int idx, bool is_pgm) {
    if (idx <= 0 || idx > key_each_row_num + key_difference_value) {
        logger->warn("Received an incorrect key value, idx is {}",idx);
        return;
    }

    if (idx < key_each_row_num) {
        handle_pgm_pvw(idx, is_pgm);
    } else if (idx == key_each_row_num) {
        update_pgm_pvw_shift_status(key_each_row_num, is_pgm);
    } else if (idx > key_each_row_num && idx < key_each_row_num + key_difference_value) {
        handle_pgm_pvw(idx, is_pgm);
    } else if (idx == key_each_row_num + key_difference_value) {
        update_pgm_pvw_shift_status(key_each_row_num, is_pgm);
    }
}

void CanFrameProcessor::handle_pgm_pvw(int idx, bool is_pgm) {
    pgm_pvw_shift_amount = handle_pgm_pvw_increment(is_pgm ? switcher->pgm_shift_status : switcher->pvw_shift_status);
    if (is_pgm) {
        switcher->xpt_pgm(idx - 1 + pgm_pvw_shift_amount);
        logger->info("send can pgm : {}", std::to_string(idx + pgm_pvw_shift_amount));
    } else {
        switcher->xpt_pvw(idx - key_difference_value -1 + pgm_pvw_shift_amount);
        logger->info("send can pvw : {}", std::to_string(idx + pgm_pvw_shift_amount - key_difference_value));
    }
}

void CanFrameProcessor::update_pgm_pvw_shift_status(int key_each_row_num, bool is_pgm) {
    int& pgm_pvw_shift_status = is_pgm ? switcher->pgm_shift_status : switcher->pvw_shift_status;
    pgm_pvw_source_sum = is_pgm ? switcher->pgm_source_sum : switcher->pvw_source_sum;

    if (pgm_pvw_source_sum < key_each_row_num) {
        logger->info("The input source is insufficient, the shift function is disabled procedure");
        return;
    }

    if (pgm_pvw_source_sum < (key_each_row_num * 2 - 1)) {pgm_pvw_shift_status = (pgm_pvw_shift_status + 1) % 2;} 
    else if (pgm_pvw_source_sum < (key_each_row_num * 3 - 2)) {pgm_pvw_shift_status = (pgm_pvw_shift_status + 1) % 3;} 
    else {pgm_pvw_shift_status = (pgm_pvw_shift_status + 1) % 4;}

    on_shift_press();
}

void CanFrameProcessor::update_key_source_shift_status(int key_each_row_num) {
    if (key_source_sum < key_each_row_num) {
        logger->info("The input source is insufficient, the shift function is disabled procedure");
        return;
    }

    if (key_source_sum < (key_each_row_num * 2 - 1)) {key_source_shift_status = (key_source_shift_status + 1) % 2;} 
    else if (key_source_sum < (key_each_row_num * 3 - 2)) {key_source_shift_status = (key_source_shift_status + 1) % 3;} 
    else {key_source_shift_status = (key_source_shift_status + 1) % 4;}

    on_shift_press();
}

int CanFrameProcessor::handle_pgm_pvw_increment(int pgm_pvw_shift_status) {
    return pgm_pvw_shift_status * (key_each_row_num - 1);
}

int CanFrameProcessor::handle_key_source_increment(int key_source_shift_status) {
    return key_source_shift_status * (key_each_row_num - 1);
}

void CanFrameProcessor::handles_clipper_key(int idx) {
    /*if (idx > 0 && idx != 7 && idx < 10) {
        switcher->nextkey(idx > 7 ? idx - 1 : idx);
    }
    if(idx == 7) {
        switcher->bkgd();
    }
    if (idx == 10){
        switcher->prevtrans();
    }*/
    if (idx == 11){
        on_key_press("mix");
    }
    /*if (idx == 12) {
        on_key_press("dip");
    }
    if (idx == 13) {
        on_key_press("luma_wipe");
    }
    if (idx == 14) {
        on_key_press("slide");
    }*/
    if (idx == 16){
        switcher->auto_();
    }
    if (idx == 17){
        switcher->cut();
    }
    if (idx == 18){
        switcher->dsk(0);
    }
    if (idx == 19){
        switcher->dsk(1);
    }
    /*if (idx > 17 && idx < 26){
        switcher->dsk(idx - 17);
    }*/
}

void CanFrameProcessor::handles_proxy_key(int idx) {
    if (idx > 0 && idx < 12) {
        switch(this->proxy_type) {
            case KEY: switcher->proxy_key_source(idx); break;
            case AUX: switcher->proxy_aux_source(idx); break;
            case UTL: switcher->proxy_macro_source(idx); break;
            case MACRO: switcher->proxy_snapshot_source(idx); break;
            case SNAPSHOT: switcher->proxy_snapshot_source(idx); break;
            case M_e: switcher->proxy_m_e_source(idx); break;
            default: logger->warn("The proxy mode is not set"); break;
        }
        /*if(this->mode_status == LongPress) {
            switch(idx) {
                case 1: switcher->proxy_key(); this->proxy_type = KEY; break;
                case 2: switcher->proxy_aux(); this->proxy_type = AUX; break;
                case 3: switcher->proxy_utl(); this->proxy_type = UTL; break;
                case 4: switcher->proxy_macro(); this->proxy_type = MACRO; break;
                case 5: switcher->proxy_snapshot(); this->proxy_type = SNAPSHOT; break;
                case 6: switcher->proxy_m_e(); this->proxy_type = M_e; break;
                default: logger->warn("The proxy mode is not set"); break;
            }
        }*/
    } else if (idx == 12) {
        //TODO: 判断MODE按键的状态是否是长按
        
        switch (mode_status) {
            case InitValue: switcher->mode(); this->mode_status = FirstValue; break;
            case FirstValue: switcher->mode(); this->mode_status = SecondValue; break;
            case SecondValue: switcher->mode(); this->mode_status = ThirdValue; break;
            case ThirdValue: switcher->mode(); this->mode_status = FourValue; break;
            case FourValue: switcher->mode(); this->mode_status = FiveValue; break;
            case FiveValue: switcher->mode(); this->mode_status == InitValue; break;
            // case LongPress: switcher->mode(); break;
            default: logger->warn("The mode button is not set"); break;
        }
    } else if (idx > 12 && idx < 24) {
        /*if (this->mode_status == LongPress) {
            return;
        }*/
        switch(this->proxy_type) {
            case KEY: switcher->proxy_source(idx - key_difference_value, "key"); break;
            case AUX: switcher->proxy_source(idx - key_difference_value, "aux"); break;
            case UTL: switcher->proxy_source(idx - key_difference_value, "macro"); break;
            case MACRO: switcher->proxy_source(idx - key_difference_value, "macro"); break;
            case SNAPSHOT: switcher->proxy_source(idx - key_difference_value, "snapshot"); break;
            case M_e: switcher->proxy_source(idx - key_difference_value, "m_e"); break;
            default: logger->warn("The mode button is not set");
        }        
    } else if (idx == 24) {
        /*if (this->mode_status == LongPress) {
            return;
        }*/
        switch(this->proxy_type) {
            case KEY: switcher->shift("key"); break;
            case AUX: switcher->shift("aux"); break;
            case UTL: switcher->shift("utl"); break;
            case MACRO: switcher->shift("macro"); break;
            case SNAPSHOT: switcher->shift("snapshot"); break;
            case M_e: switcher->shift("m_e"); break;
            default: logger->warn("The mode button is not set");
        }
    }
}

// 计算按键索引的辅助函数
int CanFrameProcessor::calculateidex(unsigned char value, int offset)
{
    for (int i = 0; i < 8; ++i){
        if (value & (1 << i)){
            return offset + i;
        }
    }
    return -1; // 默认返回值
}

// 打印 CAN 帧数据
void CanFrameProcessor::printCanFrame(const can_frame &frame, const std::string &label)
{
    logger->info("Frame from {}: ID: {}  DLC: {}  Data: ", label, std::to_string(frame.can_id), std::to_string(static_cast<int>(frame.can_dlc)));
    for (int i = 0; i < frame.can_dlc; ++i)
    {
        logger->info(" {} ", std::to_string(frame.data[i]));
    }
    logger->info("\n");
}

// 打印 CAN 帧数据
void CanFrameProcessor::printFrameData(const can_frame &frame)
{
    logger->info("receive can frame :");
    for (size_t i = 0; i < frame.can_dlc; i++)
    {
        logger->info(" {} ", std::to_string(frame.data[i]));
    }
    logger->info("\n");
}
};//namespace seeder