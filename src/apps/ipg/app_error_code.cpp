#include "app_error_code.h"
#include <string>

const char* st_app_error_category::name() const noexcept {
    return "st_app_error";
}

static std::string map_code_to_string(st_app_errc c) {
    switch (c) {
    case st_app_errc::SUCCESS: return "success";
    case st_app_errc::JSON_PARSE_FAIL: return "invalid argument";
    case st_app_errc::JSON_NOT_VALID: return "invalid argument";
    case st_app_errc::JSON_NULL: return "invalid argument";
    case st_app_errc::VIDEO_SESSION_REQUIRED: return "video arguments is required";
    case st_app_errc::AUDIO_SESSION_REQUIRED: return "audio arguments is required";
    case st_app_errc::DECKLINK_DEVICE_USED: return "SDI Device is already in use";
    case st_app_errc::ADDRESS_CONFLICT: return "IP Address or Port conflict";
    case st_app_errc::IP_ADDRESS_AND_PORT_REQUIRED: return "IP Address and Port is required";
    case st_app_errc::ID_INVALID: return "invalid argument";
    case st_app_errc::INVALID_PAYLOAD_TYPE: return "invalid payload type";
    case st_app_errc::INVALID_ADDRESS: return "IP Address or Port Invalid";
    case st_app_errc::DECKLINK_DEVICE_ID_REQUIRED: return "Device ID Required";
    case st_app_errc::MULTICAST_IP_ADDRESS_REQUIRED: return "Invalid IP Address. Multicast Address is required";
    default:
      return "未知异常";
    }
}

std::string st_app_error_category::message(int ev) const {
    std::string r;
    // r.append("错误代码");
    // r.append(std::to_string(ev));
    // r.append(":");
    r.append(map_code_to_string(static_cast<st_app_errc>(ev)));
    return r;
}

const st_app_error_category & st_app_error_category::instance() {
    static st_app_error_category value;
    return value;
}

std::error_code make_error_code(st_app_errc e) {
    return std::error_code(static_cast<int>(e), st_app_error_category::instance());
}


const char* st_app_device_error_category::name() const noexcept {
    return "st_app_device_error";
}

std::string st_app_device_error_category::message(int ev) const {
    std::string r;
    r.append("Device Error ");
    r.append(std::to_string(ev));
    return r;
}

const st_app_device_error_category & st_app_device_error_category::instance() {
    static st_app_device_error_category value;
    return value;
}