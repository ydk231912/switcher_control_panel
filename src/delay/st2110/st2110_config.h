#pragma once

#include <string>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>

#include "util/json_util.h"

namespace seeder {

namespace config {

// 网卡接口
struct ST2110DeviceConfig {
    int id;
    std::string sip;
    std::string device;
    std::string display_name;
};

struct PTPConfig {
    bool enable = true;
};

struct ST2110Config {
    bool enable = true;
    int sch_quota = 0;

    std::vector<ST2110DeviceConfig> devices;

    PTPConfig ptp;

    std::optional<ST2110DeviceConfig> find_device(int id) const {
        for (auto &d : devices) {
            if (d.id == id) {
                return d;
            }
        }
        return std::nullopt;
    }
};

struct ST2110Redundant {
    bool enable = false;
    int device_id = 0;
    std::string ip;
    int port = 0;
};

struct ST2110AudioConfig {
    bool enable = false;
    std::string ip;
    int port = 0;
    int payload_type = 0;
    std::string audio_format;
    std::string audio_sampling;
    std::string audio_channel;
    std::string audio_ptime;

    std::optional<ST2110Redundant> redundant;
};

struct ST2110VideoConfig {
    bool enable = false;
    std::string ip;
    int port = 0;
    int payload_type = 0;
    std::string video_format;
    std::string color_space;
    std::optional<ST2110Redundant> redundant;
};

struct ST2110InputConfig {
    bool enable = true;
    std::string id;
    std::string name;
    int device_id = 0;
    ST2110VideoConfig video;
    ST2110AudioConfig audio;
};

struct ST2110OutputConfig {
    bool enable = true;
    std::string id;
    std::string name;
    int device_id = 0;
    ST2110VideoConfig video;
    ST2110AudioConfig audio;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ST2110DeviceConfig,
    id,
    sip,
    device,
    display_name
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    PTPConfig,
    enable
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ST2110Config,
    enable,
    sch_quota,
    devices,
    ptp
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ST2110Redundant,
    enable,
    device_id,
    ip,
    port
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ST2110VideoConfig,
    enable,
    ip,
    port,
    payload_type,
    video_format,
    color_space,
    redundant
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ST2110AudioConfig,
    enable,
    ip,
    port,
    payload_type,
    audio_format,
    audio_sampling,
    audio_channel,
    audio_ptime,
    redundant
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ST2110InputConfig,
    enable,
    id,
    name,
    device_id,
    video,
    audio
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ST2110OutputConfig,
    enable,
    id,
    name,
    device_id,
    video,
    audio
)

} // namespace seeder::config

} // namespace seeder