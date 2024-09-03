#include <iostream>
#include <numeric>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <termios.h>
#include <ctime>
#include <stdexcept>
#include <fcntl.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <chrono>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <cerrno>

namespace seeder{

class UART {
public:
    void sendFrame(const std::vector<unsigned char>& data);

private:
    void openPort();
    void closePort();
    void configure();

};

unsigned char calculate_checksum(const std::vector<unsigned char>& data, size_t start, size_t end);

class DataFrame {
public:
    DataFrame();
    std::vector<unsigned char> construct_frame1(unsigned char frame_header, unsigned char device_id);
    std::vector<unsigned char> construct_frame2(unsigned char frame_header, unsigned char device_id);

};


class MainController{
public:
    void start();
    void stop();
    
private:
    void senderLoop();
    void printerLoop();
};

class CanSocket{
public:
    ~CanSocket();
    int getSocketFd() const;

private:
    void setupSocket();
    void setNonblocking();


};

class CanFrameProcessor{
    void startProcessing();
    void stopProcessing();

private:
    void processFrames();
    int calculateIndex(unsigned char value, int offset);
    void handle_FrameData1(const can_frame &frame, std::vector<can_frame> &buffer, std::mutex &buffer_mutex, int frame_id);
    void handle_FrameData2(const can_frame &frame, std::vector<can_frame> &buffer, std::mutex &buffer_mutex, int frame_id);
    void printCanFrame(const can_frame &frame, const std::string &label);


};
};