/**
 * rtp packet 
 *
 */
#pragma once

#include <map>
#include <memory>


namespace seeder::rtp {

class packet
{
  public:
    packet();
    packet(int length);
    ~packet();

    void set_marker(uint8_t marker);
    void set_sequence_number(uint16_t sequence_number);
    void set_timestamp(uint32_t timestamp);

    uint8_t* get_payload_ptr();
    uint8_t* get_data_ptr();

    int get_payload_size();
    int get_data_size();

    int get_size();
    void reset_size(int length);

    void set_first_of_frame(bool first_packet);
    bool get_first_of_frame();
    void set_last_of_frame(bool last_packet);

  private:
    uint8_t*    data_ = nullptr;
    int      length_;
    bool first_of_frame_ = false;
    bool last_of_frame_ = false;


};

} // namespace seeder::rtp
