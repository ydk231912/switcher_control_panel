#include "can_receive.h"

namespace seeder {

CanSocket::CanSocket(const std::string &interface) : interface_name(interface) {setupSocket();} //初始化 CAN 套接字

CanSocket::~CanSocket() {close(socket_fd);} // 关闭套接字

int CanSocket::getSocketFd() const {return socket_fd;} // 获取套接字文件描述符

// 设置并初始化 CAN 套接字
void CanSocket::setupSocket()
{
    struct sockaddr_can addr = {};

    // 创建 raw CAN 套接字
    if ((socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        logger->error("Socket creation failed", strerror(errno));
    }

    // 初始化 sockaddr_can 结构体
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(interface_name.c_str());

    if (addr.can_ifindex == 0)
    {
        logger->error("Invalid CAN interface", strerror(errno));
    }

    // 绑定套接字到 CAN 接口
    if (bind(socket_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        logger->error("Bind failed", strerror(errno));
    }

    // 设置套接字为非阻塞模式
    setNonblocking();
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


//接受 CanSocket 实例
CanFrameProcessor::CanFrameProcessor(CanSocket &canSocket, std::shared_ptr<seeder::Switcher> sw)
    : socket(canSocket), switcher(sw),stop_flag(false)
{
    logger->info("CanFrameProcessor initialized");
}

// 启动 CAN 帧处理线程
void CanFrameProcessor::startProcessing()
{
    processing_thread = std::thread(&CanFrameProcessor::processFrames, this);
}

// 停止 CAN 帧处理线程
void CanFrameProcessor::stopProcessing()
{
    stop_flag = true;
    if (processing_thread.joinable())
    {
        processing_thread.join();
    }
}

// 处理 CAN 帧数据
void CanFrameProcessor::processFrames()
{
    can_frame frame;
    int nbytes;
    // stop_flag = false;
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
            logger->error("CAN read failed", strerror(errno));
        }
    }
}

// 处理 CAN 帧数据
void CanFrameProcessor::processFrame(const can_frame &frame)
{
    logger->info("receive can frame_id : {}", std::to_string(frame.can_id));
    switch (frame.can_id)
    {
    case FRAME1_ID:
    //   logger->info("receive can frame : {}", frame.can_id);
        handle_FrameData1(frame, FRAME1_ID);
        break;
    case FRAME2_ID:
    //   logger->info("receive can frame : {}", frame.can_id);
        handle_FrameData2(frame, FRAME2_ID);
        break;
    case FRAME3_ID:
    //   logger->info("receive can frame : {}", frame.can_id);
        handle_FrameData3(frame, FRAME3_ID);
        break;
    }
}

// 处理并发送 CAN 帧数据 -- 推子板
void CanFrameProcessor::handle_FrameData1(const can_frame &frame, int frame_id)
{
    // auto now = std::chrono::system_clock::now();
    // auto duration = now.time_since_epoch();
    // int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    // 检查帧 ID 和数据有效性
    if (frame.data[0] != 0x00){
        idx = calculateidex(frame.data[0], 1);
        //key1-key6
        if (idx > 0 && idx < 7){
            switcher->nextkey(idx);
        }
        //bkgd
        else if (idx == 7){
            switcher->bkgd();
        }
        //key7
        else if (idx == 8){
            switcher->nextkey(idx - 1);
        }
        // if (idx >= 1 && idx <= 8) switcher->nextkey(idx == 8 ? idx - 1 : idx);
        // else if (idx == 7) switcher->bkgd();
    }
    else if (frame.data[1] != 0x00){
        idx = calculateidex(frame.data[1], 9);
        //key8
        if (idx == 9){
            switcher->nextkey(idx - 1);
        }
        //prevtrans
        else if (idx == 10){
            switcher->prevtrans();
        }
        //mix
        else if (idx == 11){
            switcher->mix();
        }
        // Dip、Wipe、Stinger、DVE
        else if (idx > 11 && idx < 16){
            // Dip、Wipe、Stinger、DVE
        }
        //auto
        else if (idx == 16){
            switcher->auto_();
        }
    }
    else if (frame.data[2] != 0x00){
        idx = calculateidex(frame.data[2], 17);
        //cut
        if (idx == 17){
            switcher->cut();
        }
        //dsk
        else if (idx > 17 && idx < 25){
            switcher->dsk(idx - 17);
        }
        else if (idx == 18){
            // if (milliseconds - dsk_excut_time > 1000)
            // {
                logger->info("receive can dsk : {}", std::to_string(idx));
                switcher->dsk(0);
            // }
        }
        else if (idx == 19){
            // if (milliseconds - dsk_excut_time > 1000)
            // {
                logger->info("receive can dsk : {}", std::to_string(idx));
                switcher->dsk(1);
            // }
        }
    }
    else if (frame.data[3] != 0x00){
        idx = calculateidex(frame.data[3], 25);
        if (idx == 25){
            switcher->dsk(idx - 17);
        }
    }
}

// 处理并发送 CAN 帧数据 -- 从站1 -- 疑惑？
void CanFrameProcessor::handle_FrameData2(const can_frame &frame, int frame_id)
{
    //mode -- 按一次切换到：KEY->AUX->UTL->MACRO->SNAPSHOT->M/E，长按可以对KEY、AUX、UTL、MACRO、SNAPSHOT、M/E进行选择
    if (frame.data[1] == 0x08){
        switcher->mode();
    }
    //代理键类型
    if (frame.data[0] != 0x00){
        idx = calculateidex(frame.data[0], 1);
        if (idx == 1){
            switcher->proxy_key();
        }
        else if (idx == 2){
            switcher->proxy_aux();
        }
        else if (idx == 3){
            switcher->proxy_utl();
        }
        else if (idx == 4){
            switcher->proxy_macro();
        }
        else if (idx == 5){
            switcher->proxy_snapshot();
        }
        else if (idx == 6){
            switcher->proxy_m_e();
        }
    }
    //key_source
    if (frame.data[1] != 0x00){
        idx = calculateidex(frame.data[1], 9);
        if (idx > 12 && idx < 17){
            switcher->proxy_key_source(idx - 12);
        }
    }
    else if (frame.data[2] != 0x00){
        idx = calculateidex(frame.data[2], 17);
        if (idx < 24){
            switcher->proxy_key_source(idx - 12);
        }
        else if (idx == 24){
            switcher->shift("key");
        }
    }
    //aux_source
    if (frame.data[1] != 0x00){
        idx = calculateidex(frame.data[1], 9);
        if (idx > 12 && idx < 17){
            switcher->proxy_aux_source(idx - 12);
        }
    }
    else if (frame.data[2] != 0x00){
        idx = calculateidex(frame.data[2], 17);
        if (idx < 24){
            switcher->proxy_aux_source(idx - 12);
        }
        else if (idx == 24){
            switcher->shift("aux");
        }
    }
    //macro_source
    if (frame.data[1] != 0x00){
        idx = calculateidex(frame.data[1], 9);
        if (idx > 12 && idx < 17){
            switcher->proxy_macro_source(idx - 12);
        }
    }
    else if (frame.data[2] != 0x00){
        idx = calculateidex(frame.data[2], 17);
        if (idx < 24){
            switcher->proxy_macro_source(idx - 12);
        }
        else if (idx == 24){
            switcher->shift("macro");
        }
    }
    //snapshot_source
    if (frame.data[1] != 0x00){
        idx = calculateidex(frame.data[1], 9);
        if (idx > 12 && idx < 17){
            switcher->proxy_snapshot_source(idx - 12);
        }
    }
    else if (frame.data[2] != 0x00){
        idx = calculateidex(frame.data[2], 17);
        if (idx < 24){
            switcher->proxy_snapshot_source(idx - 12);
        }
        else if (idx == 24){
            switcher->shift("snapshot");
        }
    }
    //m/e_source
    if (frame.data[1] != 0x00){
        idx = calculateidex(frame.data[1], 9);
        if (idx > 12 && idx < 17){
            switcher->proxy_m_e_source(idx - 12);
        }
    }
    else if (frame.data[2] != 0x00){
        idx = calculateidex(frame.data[2], 17);
        if (idx < 24){
            switcher->proxy_m_e_source(idx - 12);
        }
        else if (idx == 24){
            switcher->shift("me");
        }
    }
}

// 处理并发送 CAN 帧数据 -- 从站2
void CanFrameProcessor::handle_FrameData3(const can_frame &frame, int frame_id)
{
    printFrameData(frame);
    //pgm
    if (frame.data[0] != 0x00){
        idx = calculateidex(frame.data[0], 1);
        if (this->pgm == idx){
            return;
        }
        this->pgm = idx;
        switcher->pgm(this->pgm);
        printFrameData(frame);
        logger->info("receive can pgm : {}", std::to_string(this->pgm));
    }
    else if (frame.data[1] != 0x00){
        idx = calculateidex(frame.data[1], 9);
        if (idx < 12){
            if (this->pgm == idx){
                return;
            }
            this->pgm = idx;
            switcher->pgm(this->pgm);
            printFrameData(frame);
            logger->info("receive can pgm : {}", std::to_string(this->pgm));
        }
        else if (idx == 12){
            switcher->shift("pgm");
        }
        else{
            if (this->pvw == idx){
                return;
            }
            this->pvw = idx;
            switcher->pvw(this->pvw - 12);
            printFrameData(frame);
            logger->info("receive can pvw : {}", std::to_string(this->pvw));
        }
    }
    //pvw
    else if (frame.data[2] != 0x00)
    {
        idx = calculateidex(frame.data[2], 17);
        if (idx < 24){
            if (this->pvw == idx){
                return;
            }
            this->pvw = idx;
            switcher->pvw(this->pvw - 12);
            printFrameData(frame);
            logger->info("receive can pvw : {}", std::to_string(this->pvw));
        }
        else{
            switcher->shift("pvw");
        }
    }
}

// 计算按键索引的辅助函数
int CanFrameProcessor::calculateidex(unsigned char value, int offset)
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