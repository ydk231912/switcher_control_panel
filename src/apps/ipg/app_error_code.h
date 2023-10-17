#pragma once

#include <system_error>

/**
error code enum
*/
enum class st_app_errc {
  SUCCESS = 0,
  JSON_PARSE_FAIL, // 配置文件参数解析异常
  JSON_NOT_VALID, // 配置文件参数不正确
  JSON_NULL, // 配置文件参数不正确
  VIDEO_SESSION_REQUIRED, // 缺少video session的配置
  AUDIO_SESSION_REQUIRED, // 缺少audio session的配置,
  DECKLINK_DEVICE_USED, // decklink device 已被使用
  ADDRESS_CONFLICT, // 地址冲突，ip地址和端口号已被占用
  INVALID_ADDRESS, // ip address 或者 port 不正确
  IP_ADDRESS_AND_PORT_REQUIRED,
  MULTICAST_IP_ADDRESS_REQUIRED, 
  ID_INVALID, // id不存在
  INVALID_PAYLOAD_TYPE, // payload type 参数不正确
  PAYLOAD_TYPE_REQUIRED,
  DECKLINK_DEVICE_ID_REQUIRED,
  UNSUPPORTED_VIDEO_FORMAT
};

namespace std
{
    template <> 
    struct is_error_code_enum<st_app_errc> : true_type
    {
    };
}

std::error_code make_error_code(st_app_errc e);

class st_app_error_category : public std::error_category
{
public:
  const char* name() const noexcept override;
  std::string message(int ev) const override;

  static const st_app_error_category & instance();

private:
  st_app_error_category() = default;
};

class st_app_device_error_category : public std::error_category
{
public:
  const char* name() const noexcept override;
  std::string message(int ev) const override;

  static const st_app_device_error_category & instance();
private:
  st_app_device_error_category() = default;
};