#pragma once

#include <memory>
#include <string>
#include <mutex>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "net/udp_client.h"
#include "rtp/packet.h"
#include "util/frame.h"

namespace seeder
{
    class rtp_st2110
    {
      public:
        rtp_st2110();
        ~rtp_st2110();

        /**
         * @brief send st2110 rtp package by udp
         * 
         * @return int 
         */
        int rtp_handler(util::frame frm);
        int send_rtp_8bit(AVFrame* frame);

        /**
         * @brief get system ptp time
         * 
         */
        int get_timestamp();

        void push_frame(util::frame frm);
        util::frame receive_frame();

        void push_packet(std::shared_ptr<rtp::packet> packet);
        std::shared_ptr<rtp::packet> receive_packet();



      private:
        uint16_t sequence_number_;
        std::mutex frame_mutex_, packet_mutex_;
        std::deque<util::frame> frame_buffer_;
        std::deque<std::shared_ptr<rtp::packet>> packet_buffer_;
        const std::size_t packet_capacity_ = 300000; // 300k * 1.4k = 420M 


        // test
        bool is_alloc = false;
        uint8_t* image_;
        net::udp_client client_;
    };
}