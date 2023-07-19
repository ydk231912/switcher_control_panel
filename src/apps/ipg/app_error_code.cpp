#include "app_error_code.h"
#include <string>

const char* st_app_error_category::name() const noexcept {
    return "st_app_error";
}

static std::string map_code_to_string(st_app_errc c) {
    switch (c) {
    case st_app_errc::SUCCESS: return "成功";
    case st_app_errc::JSON_PARSE_FAIL: return "配置文件参数解析异常";
    case st_app_errc::JSON_NOT_VALID: return "配置文件参数不正确";
    case st_app_errc::JSON_NULL: return "配置文件参数不正确";
    case st_app_errc::VIDEO_SESSION_REQUIRED: return "缺少video配置";
    case st_app_errc::AUDIO_SESSION_REQUIRED: return "缺少audio配置";
    case st_app_errc::DECKLINK_DEVICE_USED: return "SDI设备已被占用";
    case st_app_errc::ADDRESS_CONFLICT: return "地址冲突，IP地址或端口号已被占用";
    case st_app_errc::ID_INVALID: return "配置ID不存在";
    default:
      return "未知异常";
    }
}

std::string st_app_error_category::message(int ev) const {
    std::string r;
    r.append("错误代码");
    r.append(std::to_string(ev));
    r.append(":");
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
    r.append("设备异常 错误代码");
    r.append(std::to_string(ev));
    return r;
}

const st_app_device_error_category & st_app_device_error_category::instance() {
    static st_app_device_error_category value;
    return value;
}