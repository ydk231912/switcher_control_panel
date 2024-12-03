#pragma once 

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <json.hpp>

namespace seeder{

namespace config{

struct UrlConfig{
    std::string http_url = "";
    std::string ws_url_panel_status = "";
    std::string ws_url_config_update = "";
    std::string screen_path = "";
};

struct AutoConfig{
    int auto_transition_frame_num = 0;
    int auto_transition_sec_num = 0;
};

struct Httptimeout{
    int http_connection_timeout = 0;
    int http_read_timeout = 0;
    int http_write_timeout = 0;
};

struct PanelConfig{
    int switcher_keyboard_id = 0;
    size_t screen_data_size = 0;
    int num_screen = 0;
    int num_slave = 0;
    size_t oled_frame_size = 0;
    size_t main_ctrl_keys_size = 0;
    size_t slave_keys_size = 0;
    size_t key_frame_size = 0;
    int key_each_row_num = 0;
    int key_difference_value = 0;
    int slave_key_each_row_num = 0;
    int max_source_num = 0;

    std::string uart_port1 = "/dev/ttyS1";
    std::string uart_port2 = "/dev/ttyS3";
    int uart_baudrate = 0;
    std::string can_port = "can0";
    std::vector<int> can_frame_ids;
};

struct ScreenConfig{
    std::vector<std::string> pgm_name;
    std::vector<std::string> pvw_name;

    std::vector<std::string> proxy_keys_name;
    std::vector<std::string> proxy_sources_name;
    std::vector<std::string> fill_name;
};

struct KeyStatusConfig{
    std::vector<int> keys;
    int bkgd_key = 0;
    std::string toggle_effect_keys = "";
    int prvtrans_key = 0;
    int auto_key = 0;
    int cut_key = 0;
    std::vector<int> keys_trans;

    int mode_key = 0;
    std::vector<std::string> surrogate_keys;
    std::vector<std::string> surrogate_source_keys;

    int pgm_keys = 0;
    int pvw_keys = 0;

    std::vector<std::string> shift_keys;
};

struct ClipperPercentage{
    uint16_t percentage_value = 0;
};

struct Config{
    UrlConfig url_config;
    AutoConfig auto_config;
    Httptimeout http_timeout;
    PanelConfig panel_config;
    ScreenConfig screen_config;
    KeyStatusConfig key_status_config;
};
};// namespace config
} // namespace seeder