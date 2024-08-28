#include "config_util.h"

#include <fmt/format.h>
#include <boost/asio/ip/address.hpp>

#include "st2110/st2110_config.h"

namespace asio = boost::asio;

namespace seeder {

namespace config {

namespace {

#define THROW_VALIDATE_ERROR(...) \
    throw ValidationException(fmt::format(__VA_ARGS__))

void validate_payload_type(int payload_type) {
    /* 7 bits payload type define in RFC3550 */
    // (0, 127)
    if (!(payload_type > 0 && payload_type < 0x7F)) {
        THROW_VALIDATE_ERROR("invalid payload_type: {}", payload_type);
    }
}

void validate_multicast_address(const std::string &address) {
    boost::system::error_code ec;
    auto addr = asio::ip::make_address_v4(address, ec);
    if (ec || !addr.is_multicast()) {
        THROW_VALIDATE_ERROR("invalid ip address: {}", address);
    }
}

void validate_udp_port(int udp_port) {
    if (udp_port <= 0 || udp_port > 65535) {
        THROW_VALIDATE_ERROR("invalid port: {}", udp_port);
    }
}

void validate_st2110_audio(const seeder::config::ST2110AudioConfig &audio) {
    if (!audio.enable) {
        return;
    }
    validate_multicast_address(audio.ip);
    validate_udp_port(audio.port);
    validate_payload_type(audio.payload_type);
}

void validate_st2110_video(const seeder::config::ST2110VideoConfig &video) {
    if (!video.enable) {
        return;
    }
    validate_multicast_address(video.ip);
    validate_udp_port(video.port);
    validate_payload_type(video.payload_type);
}

void compare_redundant(
    ConfigCompareResult &result,
    const std::optional<seeder::config::ST2110Redundant> &old_value,
    const std::optional<seeder::config::ST2110Redundant> &new_value) {
    if (old_value.has_value() != new_value.has_value()) {
        result.format_changed = true;
        return;
    }
    if (old_value.has_value() && new_value.has_value()) {
        if (old_value->enable != new_value->enable) {
            result.format_changed = true;
        }
        if (old_value->device_id != new_value->device_id) {
            result.format_changed = true;
        }
        if (old_value->ip != new_value->ip) {
            result.address_changed = true;
        }
        if (old_value->port != new_value->port) {
            result.address_changed = true;
        }
    }
}

ConfigCompareResult compare_video(
    const seeder::config::ST2110VideoConfig &old_value,
    const seeder::config::ST2110VideoConfig &new_value) {
    ConfigCompareResult result;
    if (old_value.enable != new_value.enable) {
        result.format_changed = true;
    }
    if (old_value.video_format != new_value.video_format) {
        result.format_changed = true;
    }
    if (old_value.ip != new_value.ip) {
        result.address_changed = true;
    }
    if (old_value.port != new_value.port) {
        result.address_changed = true;
    }
    if (old_value.payload_type != new_value.payload_type) {
        result.format_changed = true;
    }
    compare_redundant(result, old_value.redundant, new_value.redundant);
    return result;
}

ConfigCompareResult compare_audio(
    const seeder::config::ST2110AudioConfig &old_value,
    const seeder::config::ST2110AudioConfig &new_value) {
    ConfigCompareResult result;
    if (old_value.enable != new_value.enable) {
        result.format_changed = true;
    }
    if (old_value.ip != new_value.ip) {
        result.address_changed = true;
    }
    if (old_value.port != new_value.port) {
        result.address_changed = true;
    }
    if (old_value.payload_type != new_value.payload_type) {
        result.format_changed = true;
    }
    if (old_value.audio_format != new_value.audio_format) {
        result.format_changed = true;
    }
    if (old_value.audio_channel != new_value.audio_channel) {
        result.format_changed = true;
    }
    if (old_value.audio_ptime != new_value.audio_ptime) {
        result.format_changed = true;
    }
    compare_redundant(result, old_value.redundant, new_value.redundant);
    return result;
}

}  // namespace

void validate_input(const seeder::config::ST2110InputConfig &input) {
    validate_st2110_video(input.video);
    validate_st2110_audio(input.audio);
}

void validate_output(const seeder::config::ST2110OutputConfig &output) {
    validate_st2110_video(output.video);
    validate_st2110_audio(output.audio);
}

ConfigCompareResult compare_input(
    const seeder::config::ST2110InputConfig &old_value,
    const seeder::config::ST2110InputConfig &new_value) {
    ConfigCompareResult result;
    if (old_value.name != new_value.name) {
        result.name_changed = true;
    }
    if (old_value.device_id != new_value.device_id) {
        result.format_changed = true;
    }
    result.merge(compare_video(old_value.video, new_value.video));
    result.merge(compare_audio(old_value.audio, new_value.audio));
    return result;
}

ConfigCompareResult compare_output(
    const seeder::config::ST2110OutputConfig &old_value,
    const seeder::config::ST2110OutputConfig &new_value) {
    ConfigCompareResult result;
    if (old_value.name != new_value.name) {
        result.name_changed = true;
    }
    if (old_value.device_id != new_value.device_id) {
        result.format_changed = true;
    }
    result.merge(compare_video(old_value.video, new_value.video));
    result.merge(compare_audio(old_value.audio, new_value.audio));
    return result;
}

}  // namespace config

}  // namespace seeder