/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */

#include "parse_json.h"

#include "app_base.h"
#include "core/util/logger.h"
#include "core/util/uuid.h"
#include "core/util/fs.h"
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <json/reader.h>
#include <json/value.h>
#include <json_object.h>
#include <json_tokener.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <json-c/json.h>
#include <vector>
#include "core/video_format.h"
#include "decklink/manager.h"
#include "app_error_code.h"


using namespace seeder::core;

#if (JSON_C_VERSION_NUM >= ((0 << 16) | (13 << 8) | 0)) || \
    (JSON_C_VERSION_NUM < ((0 << 16) | (10 << 8) | 0))
static inline json_object* st_json_object_object_get(json_object* obj, const char* key) {
  return json_object_object_get(obj, key);
}
#else
static inline json_object* st_json_object_object_get(json_object* obj, const char* key) {
  json_object* value;
  int ret = json_object_object_get_ex(obj, key, &value);
  if (ret) return value;
  logger->error("{}, can not get object with key: {}!", __func__, key);
  return NULL;
}
#endif

#define ERRC_FAILED(c) (c) != st_app_errc::SUCCESS
#define ERRC_EXPECT_SUCCESS(c) if (ERRC_FAILED(c)) return (c)

static const struct st_video_fmt_desc st_video_fmt_descs[] = {
    {
        .fmt = VIDEO_FORMAT_480I_59FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i480i59",
        .width = 720,
        .height = 480,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_576I_50FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::pal,
        .name = "i576i50",
        .width = 720,
        .height = 576,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_720P_119FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i720p119",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P119_88,
    },
    {
        .fmt = VIDEO_FORMAT_720P_59FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x720p5994,
        .name = "i720p59",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_720P_50FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x720p5000,
        .name = "i720p50",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_720P_29FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x720p2997,
        .name = "i720p29",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P29_97,
    },
    {
        .fmt = VIDEO_FORMAT_720P_25FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x720p2500,
        .name = "i720p25",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P25,
    },
    {
        .fmt = VIDEO_FORMAT_720P_60FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x720p6000,
        .name = "i720p60",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P60,
    },
    {
        .fmt = VIDEO_FORMAT_720P_30FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x720p3000,
        .name = "i720p30",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P30,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_119FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i1080p119",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P119_88,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_59FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x1080p5994,
        .name = "i1080p59",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_50FPS,
        .core_fmt = seeder::core::video_fmt::x1080p5000,
        .name = "i1080p50",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_29FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x1080p2997,
        .name = "i1080p29",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P29_97,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_25FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x1080p2500,
        .name = "i1080p25",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P25,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_60FPS,
        // .core_fmt = seeder::core::video_fmt::invalid,
        .core_fmt = seeder::core::video_fmt::x1080p6000,
        .name = "i1080p60",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P60,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_30FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x1080p3000,
        .name = "i1080p30",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P30,
    },
    {
        .fmt = VIDEO_FORMAT_1080I_59FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x1080i5994,
        .name = "i1080i59",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_1080I_50FPS,
        // .core_fmt = seeder::core::video_fmt::invalid,
        .core_fmt = seeder::core::video_fmt::x1080i5000,
        .name = "i1080i50",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_119FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i2160p119",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P119_88,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_59FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x2160p5994,
        .name = "i2160p59",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_50FPS,
        .core_fmt = seeder::core::video_fmt::x2160p5000,
        .name = "i2160p50",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_29FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x2160p2997,
        .name = "i2160p29",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P29_97,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_25FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x2160p2500,
        .name = "i2160p25",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P25,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_60FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x2160p6000,
        .name = "i2160p60",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P60,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_30FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        // .core_fmt = seeder::core::video_fmt::x2160p3000,
        .name = "i2160p30",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P30,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_119FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i4320p119",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P119_88,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_59FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i4320p59",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_50FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i4320p50",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_29FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i4320p29",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P29_97,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_25FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i4320p25",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P25,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_60FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i4320p60",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P60,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_30FPS,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "i4320p30",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P30,
    },
    {
        .fmt = VIDEO_FORMAT_AUTO,
        .core_fmt = seeder::core::video_fmt::invalid,
        .name = "auto",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P59_94,
    },
};

static std::vector<st_video_fmt_desc> st_app_init_video_fmts() {
  std::vector<st_video_fmt_desc> v;
  for (int i = 0; i < ARRAY_SIZE(st_video_fmt_descs); i++) {
    if (st_video_fmt_descs[i].core_fmt != seeder::core::video_fmt::invalid) {
      v.push_back(st_video_fmt_descs[i]);
    }
  }
  return std::vector<st_video_fmt_desc>(v.rbegin(), v.rend());
}

static const std::vector<st_video_fmt_desc> & st_app_get_video_fmts() {
  static std::vector<st_video_fmt_desc> fmts = st_app_init_video_fmts();
  return fmts;
}

seeder::core::video_fmt st_app_get_core_video_fmt(enum video_format fmt) {
  for (auto &f : st_app_get_video_fmts()) {
    if (f.fmt == fmt) {
      return f.core_fmt;
    }
  }
  return seeder::core::video_fmt::invalid;
}

enum video_format st_app_core_video_fmt_to_video_format(seeder::core::video_fmt fmt) {
  for (auto &f : st_app_get_video_fmts()) {
    if (f.core_fmt == fmt) {
      return f.fmt;
    }
  }
  logger->error("st_app_core_video_fmt_to_video_format invalid format: {}", (int) fmt);
  throw std::runtime_error("st_app_core_video_fmt_to_video_format");
}

static std::unordered_map<std::string, st_video_fmt_desc> init_fmt_core_name_map() {
  std::unordered_map<std::string, st_video_fmt_desc> name_map;
  for (auto &f : st_app_get_video_fmts()) {
    name_map.emplace(seeder::core::video_format_desc::get(f.core_fmt).name, f);
  }
  return name_map;
}

static const st_video_fmt_desc * get_video_fmt_from_core_fmt_name(const std::string &name) {
  static std::unordered_map<std::string, st_video_fmt_desc> name_map = init_fmt_core_name_map();
  auto it = name_map.find(name);
  if (it != name_map.end()) {
    return &it->second;
  }
  return nullptr;
}

#define VNAME(name) (#name)

#define REQUIRED_ITEM(string)                                 \
  do {                                                        \
    if (string == NULL) {                                     \
      logger->error("{}, can not parse {}", __func__, VNAME(string)); \
      return st_app_errc::JSON_PARSE_FAIL;                             \
    }                                                         \
  } while (0)

/* 7 bits payload type define in RFC3550 */
static inline bool st_json_is_valid_payload_type(int payload_type) {
  if (payload_type > 0 && payload_type < 0x7F)
    return true;
  else
    return false;
}

static st_app_errc parse_ip_addr_str(const char *ip_addr_str, void *buf) {
  if (inet_pton(AF_INET, ip_addr_str, buf) != 1) {
    if (strlen(ip_addr_str) == 0) {
      return st_app_errc::IP_ADDRESS_AND_PORT_REQUIRED;
    }
    logger->warn("failed to parse ip address {}", ip_addr_str);
    return st_app_errc::INVALID_ADDRESS;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc parse_multicast_ip_addr_str(const char *ip_addr_str, void *buf) {
  if (!ip_addr_str) {
    return st_app_errc::IP_ADDRESS_AND_PORT_REQUIRED;
  }
  if (inet_pton(AF_INET, ip_addr_str, buf) != 1) {
    if (strlen(ip_addr_str) == 0) {
      return st_app_errc::IP_ADDRESS_AND_PORT_REQUIRED;
    }
    logger->warn("failed to parse ip address {}", ip_addr_str);
    return st_app_errc::INVALID_ADDRESS;
  }
  auto addr_buf = reinterpret_cast<uint8_t *>(buf);
  if (addr_buf[0] && !(addr_buf[0] >= 224 && addr_buf[0] <= 239)) {
    return st_app_errc::MULTICAST_IP_ADDRESS_REQUIRED;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc st_json_parse_interfaces(json_object* interface_obj,
                                    st_json_interface_t* interface) {
  if (interface_obj == NULL || interface == NULL) {
    logger->error("{}, can not parse interfaces!", __func__);
    return st_app_errc::JSON_NULL;
  }

  const char* name =
      json_object_get_string(st_json_object_object_get(interface_obj, "name"));
  REQUIRED_ITEM(name);
  snprintf(interface->name, sizeof(interface->name), "%s", name);

  const char* ip = json_object_get_string(st_json_object_object_get(interface_obj, "ip"));
  if (ip) {
    interface->ip_addr_str = ip;
    if (!interface->ip_addr_str.empty()) {
      st_app_errc ret = parse_ip_addr_str(ip, interface->ip_addr);
      ERRC_EXPECT_SUCCESS(ret);
    }
  }

  return st_app_errc::SUCCESS;
}

static st_app_errc parse_array_device_id(json_object* obj,st_app_tx_output_sdi* base)
{
  int device_id = json_object_get_int(st_json_object_object_get(obj, "device_id"));
  if(device_id < 0)
  {
    return st_app_errc::JSON_PARSE_FAIL;
  }
  base->device_id = device_id;
  return st_app_errc::SUCCESS;
}

static void parse_array_enable(json_object* obj,st_app_tx_output_sdi* base)
{
  json_object* enable_obj=st_json_object_object_get(obj, "enable");
  bool enable = true;
  if(enable_obj)
  {
    enable = json_object_get_boolean(enable_obj);
  }
  base->enable = enable;
}

static st_app_errc parse_udp_port(json_object *obj, uint16_t &out_port) {
  if (!obj) {
    return st_app_errc::IP_ADDRESS_AND_PORT_REQUIRED;
  }
  int port = json_object_get_int(obj);
  if (port <= 0 || port > 65535) {
    logger->error("{}, invalid start port {}", __func__, port);
    if (port == 0) {
      return st_app_errc::IP_ADDRESS_AND_PORT_REQUIRED;
    }
    return st_app_errc::INVALID_ADDRESS;
  }
  out_port = port;

  return st_app_errc::SUCCESS;
}

static st_app_errc parse_base_udp_port(json_object* obj, st_json_session_base_t* base) {
  return parse_udp_port(st_json_object_object_get(obj, "start_port"), base->udp_port[0]);
}

static st_app_errc parse_base_payload_type(json_object* obj, st_json_session_base_t* base) {
  json_object* payload_type_object = st_json_object_object_get(obj, "payload_type");
  if (payload_type_object) {
    base->payload_type = json_object_get_int(payload_type_object);
    if (base->payload_type == 0) {
      return st_app_errc::PAYLOAD_TYPE_REQUIRED;
    }
    if (!st_json_is_valid_payload_type(base->payload_type)) {
      logger->error("{}, invalid payload type {}", __func__, base->payload_type);
      return st_app_errc::INVALID_PAYLOAD_TYPE;
    }
  } else {
    return st_app_errc::PAYLOAD_TYPE_REQUIRED;
  }

  return st_app_errc::SUCCESS;
}

static void parse_base_enable(json_object* obj, st_json_session_base_t* base) {
  json_object* enable_obj = st_json_object_object_get(obj, "enable");
  bool enable = true;
  if (enable_obj) {
    enable = json_object_get_boolean(enable_obj);
  }
  base->enable = enable;
}

static st_app_errc parse_video_type(json_object* video_obj, st_json_video_session_t* video) {
  const char* type = json_object_get_string(st_json_object_object_get(video_obj, "type"));
  REQUIRED_ITEM(type);
  if (strcmp(type, "frame") == 0) {
    video->info.type = ST20_TYPE_FRAME_LEVEL;
  } else if (strcmp(type, "rtp") == 0) {
    video->info.type = ST20_TYPE_RTP_LEVEL;
  } else if (strcmp(type, "slice") == 0) {
    video->info.type = ST20_TYPE_SLICE_LEVEL;
  } else {
    logger->error("{}, invalid video type {}", __func__, type);
    return st_app_errc::JSON_NOT_VALID;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc parse_video_pacing(json_object* video_obj, st_json_video_session_t* video) {
  const char* pacing =
      json_object_get_string(st_json_object_object_get(video_obj, "pacing"));
  REQUIRED_ITEM(pacing);
  if (strcmp(pacing, "gap") == 0) {
    video->info.pacing = PACING_GAP;
  } else if (strcmp(pacing, "linear") == 0) {
    video->info.pacing = PACING_LINEAR;
  } else {
    logger->error("{}, invalid video pacing {}", __func__, pacing);
    return st_app_errc::JSON_NOT_VALID;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc parse_video_packing(json_object* video_obj, st_json_video_session_t* video) {
  const char* packing =
      json_object_get_string(st_json_object_object_get(video_obj, "packing"));
  if (packing) {
    if (strcmp(packing, "GPM_SL") == 0) {
      video->info.packing = ST20_PACKING_GPM_SL;
    } else if (strcmp(packing, "BPM") == 0) {
      video->info.packing = ST20_PACKING_BPM;
    } else if (strcmp(packing, "GPM") == 0) {
      video->info.packing = ST20_PACKING_GPM;
    } else {
      logger->error("{}, invalid video packing mode {}", __func__, packing);
      return st_app_errc::JSON_NOT_VALID;
    }
  } else { /* default set to bpm */
    video->info.packing = ST20_PACKING_BPM;
  }
  return st_app_errc::SUCCESS;
}

static void parse_video_change_interlace(json_object* video_obj, st_json_video_session_t* video) {
  json_object *obj = st_json_object_object_get(video_obj, "change_interlace");
  if (obj) {
    video->info.change_interlace = json_object_get_boolean(obj);
  }
}

static st_app_errc parse_video_tr_offset(json_object* video_obj, st_json_video_session_t* video) {
  const char* tr_offset =
      json_object_get_string(st_json_object_object_get(video_obj, "tr_offset"));
  REQUIRED_ITEM(tr_offset);
  if (strcmp(tr_offset, "default") == 0) {
    video->info.tr_offset = TR_OFFSET_DEFAULT;
  } else if (strcmp(tr_offset, "none") == 0) {
    video->info.tr_offset = TR_OFFSET_NONE;
  } else {
    logger->error("{}, invalid video tr_offset {}", __func__, tr_offset);
    return st_app_errc::JSON_NOT_VALID;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc st_json_parse_tx_outputsdi(json_object* object,
              st_app_tx_output_sdi* outputsdi)
{
  if(object == NULL || outputsdi == NULL)
  {
    logger->error("{}, can not parse tx outputsdi session", __func__);
    return st_app_errc::JSON_NULL;
  }
  st_app_errc ret;

  /* parse device_id */
  ret = parse_array_device_id(object,outputsdi);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse enable */
  parse_array_enable(object,outputsdi);

  return st_app_errc::SUCCESS;
}

static st_app_errc parse_video_format(json_object* video_obj, st_json_video_session_t* video) {
  const char* video_format =
      json_object_get_string(st_json_object_object_get(video_obj, "video_format"));
  REQUIRED_ITEM(video_format);
  int i;
  for (i = 0; i < ARRAY_SIZE(st_video_fmt_descs); i++) {
    if (strcmp(video_format, st_video_fmt_descs[i].name) == 0) {
      video->info.video_format = st_video_fmt_descs[i].fmt;
      return st_app_errc::SUCCESS;
    }
  }
  logger->error("{}, invalid video format {}", __func__, video_format);
  return st_app_errc::JSON_NOT_VALID;
}

static st_app_errc parse_video_pg_format(json_object* video_obj, st_json_video_session_t* video) {
  const char* pg_format =
      json_object_get_string(st_json_object_object_get(video_obj, "pg_format"));
  REQUIRED_ITEM(pg_format);
  if (strcmp(pg_format, "YUV_422_10bit") == 0) {
    video->info.pg_format = ST20_FMT_YUV_422_10BIT;
  } else if (strcmp(pg_format, "YUV_422_8bit") == 0) {
    video->info.pg_format = ST20_FMT_YUV_422_8BIT;
  } else if (strcmp(pg_format, "YUV_422_12bit") == 0) {
    video->info.pg_format = ST20_FMT_YUV_422_12BIT;
  } else if (strcmp(pg_format, "YUV_422_16bit") == 0) {
    video->info.pg_format = ST20_FMT_YUV_422_16BIT;
  } else if (strcmp(pg_format, "YUV_420_8bit") == 0) {
    video->info.pg_format = ST20_FMT_YUV_420_8BIT;
  } else if (strcmp(pg_format, "YUV_420_10bit") == 0) {
    video->info.pg_format = ST20_FMT_YUV_420_10BIT;
  } else if (strcmp(pg_format, "YUV_420_12bit") == 0) {
    video->info.pg_format = ST20_FMT_YUV_420_12BIT;
  } else if (strcmp(pg_format, "RGB_8bit") == 0) {
    video->info.pg_format = ST20_FMT_RGB_8BIT;
  } else if (strcmp(pg_format, "RGB_10bit") == 0) {
    video->info.pg_format = ST20_FMT_RGB_10BIT;
  } else if (strcmp(pg_format, "RGB_12bit") == 0) {
    video->info.pg_format = ST20_FMT_RGB_12BIT;
  } else if (strcmp(pg_format, "RGB_16bit") == 0) {
    video->info.pg_format = ST20_FMT_RGB_16BIT;
  } else {
    logger->error("{}, invalid pixel group format {}", __func__, pg_format);
    return st_app_errc::JSON_NOT_VALID;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc parse_url(json_object* obj, const char* name, char* url) {
  const char* url_src = json_object_get_string(st_json_object_object_get(obj, name));
  REQUIRED_ITEM(url_src);
  snprintf(url, ST_APP_URL_MAX_LEN, "%s", url_src);
  return st_app_errc::SUCCESS;
}

static st_app_errc st_json_parse_tx_video(json_object* video_obj, st_json_video_session_t* video) {
  if (video_obj == NULL || video == NULL) {
    logger->error("{}, can not parse tx video session", __func__);
    return st_app_errc::JSON_NULL;
  }
  st_app_errc ret;
  /* parse udp port  */
  ret = parse_base_udp_port(video_obj, &video->base);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse payload type */
  ret = parse_base_payload_type(video_obj, &video->base);
  ERRC_EXPECT_SUCCESS(ret);

  parse_base_enable(video_obj, &video->base);

  /* parse video type */
  /* ret = parse_video_type(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret); */

  /* parse video pacing */
  /* ret = parse_video_pacing(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret); */

  /* parse video packing mode */
  ret = parse_video_packing(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret);

  parse_video_change_interlace(video_obj, video);

  /* parse tr offset */
  /* ret = parse_video_tr_offset(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret); */

  /* parse video format */
  // ret = parse_video_format(video_obj, video);
  // ERRC_EXPECT_SUCCESS(ret);

  /* parse pixel group format */
  /* ret = parse_video_pg_format(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret); */

  return st_app_errc::SUCCESS;
}



static st_app_errc st_json_parse_rx_video(json_object* video_obj, st_json_video_session_t* video) {
  if (video_obj == NULL || video == NULL) {
    logger->error("{}, can not parse rx video session", __func__);
    return st_app_errc::JSON_NULL;
  }
  st_app_errc ret;

  /* parse udp port  */
  ret = parse_base_udp_port(video_obj, &video->base);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse payload type */
  ret = parse_base_payload_type(video_obj, &video->base);
  ERRC_EXPECT_SUCCESS(ret);

  parse_base_enable(video_obj, &video->base);

  /* parse video type */
  /* ret = parse_video_type(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret); */

  /* parse video pacing */
  /* ret = parse_video_pacing(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret); */

  /* parse tr offset */
  /* ret = parse_video_tr_offset(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret); */

  /* parse video format */
  // ret = parse_video_format(video_obj, video);
  // ERRC_EXPECT_SUCCESS(ret);

  /* parse pixel group format */
  /* ret = parse_video_pg_format(video_obj, video);
  ERRC_EXPECT_SUCCESS(ret); */

  return st_app_errc::SUCCESS;
}

static st_app_errc parse_audio_type(json_object* audio_obj, st_json_audio_session_t* audio) {
  const char* type = json_object_get_string(st_json_object_object_get(audio_obj, "type"));
  REQUIRED_ITEM(type);
  if (strcmp(type, "frame") == 0) {
    audio->info.type = ST30_TYPE_FRAME_LEVEL;
  } else if (strcmp(type, "rtp") == 0) {
    audio->info.type = ST30_TYPE_RTP_LEVEL;
  } else {
    logger->error("{}, invalid audio type {}", __func__, type);
    return st_app_errc::JSON_NOT_VALID;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc parse_audio_format(json_object* audio_obj, st_json_audio_session_t* audio) {
  const char* audio_format =
      json_object_get_string(st_json_object_object_get(audio_obj, "audio_format"));
  REQUIRED_ITEM(audio_format);
  if (strcmp(audio_format, "PCM8") == 0) {
    audio->info.audio_format = ST30_FMT_PCM8;
  } else if (strcmp(audio_format, "PCM16") == 0) {
    audio->info.audio_format = ST30_FMT_PCM16;
  } else if (strcmp(audio_format, "PCM24") == 0) {
    audio->info.audio_format = ST30_FMT_PCM24;
  } else if (strcmp(audio_format, "AM824") == 0) {
    audio->info.audio_format = ST31_FMT_AM824;
  } else {
    logger->error("{}, invalid audio format {}", __func__, audio_format);
    return st_app_errc::JSON_NOT_VALID;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc parse_audio_channel(json_object* audio_obj, st_json_audio_session_t* audio) {
  json_object* audio_channel_array =
      st_json_object_object_get(audio_obj, "audio_channel");
  if (audio_channel_array == NULL ||
      json_object_get_type(audio_channel_array) != json_type_array) {
    logger->error("{}, can not parse audio channel", __func__);
    return st_app_errc::JSON_PARSE_FAIL;
  }
  audio->info.audio_channel = 0; /* reset channel number*/
  for (int i = 0; i < json_object_array_length(audio_channel_array); ++i) {
    json_object* channel_obj = json_object_array_get_idx(audio_channel_array, i);
    const char* channel = json_object_get_string(channel_obj);
    REQUIRED_ITEM(channel);
    if (strcmp(channel, "M") == 0) {
      audio->info.audio_channel += 1;
    } else if (strcmp(channel, "DM") == 0 || strcmp(channel, "ST") == 0 ||
               strcmp(channel, "LtRt") == 0 || strcmp(channel, "AES3") == 0) {
      audio->info.audio_channel += 2;
    } else if (strcmp(channel, "51") == 0) {
      audio->info.audio_channel += 6;
    } else if (strcmp(channel, "71") == 0) {
      audio->info.audio_channel += 8;
    } else if (strcmp(channel, "222") == 0) {
      audio->info.audio_channel += 24;
    } else if (strcmp(channel, "SGRP") == 0) {
      audio->info.audio_channel += 4;
    } else if (channel[0] == 'U' && channel[1] >= '0' && channel[1] <= '9' &&
               channel[2] >= '0' && channel[2] <= '9' && channel[3] == '\0') {
      int num_channel = (channel[1] - '0') * 10 + (channel[2] - '0');
      if (num_channel < 1 || num_channel > 64) {
        logger->error("{}, audio undefined channel number out of range {}", __func__, channel);
        return st_app_errc::JSON_NOT_VALID;
      }
      audio->info.audio_channel += num_channel;
    } else {
      logger->error("{}, invalid audio channel {}", __func__, channel);
      return st_app_errc::JSON_NOT_VALID;
    }
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc parse_audio_sampling(json_object* audio_obj, st_json_audio_session_t* audio) {
  const char* audio_sampling =
      json_object_get_string(st_json_object_object_get(audio_obj, "audio_sampling"));
  REQUIRED_ITEM(audio_sampling);
  if (strcmp(audio_sampling, "48kHz") == 0) {
    audio->info.audio_sampling = ST30_SAMPLING_48K;
  } else if (strcmp(audio_sampling, "96kHz") == 0) {
    audio->info.audio_sampling = ST30_SAMPLING_96K;
  } else if (strcmp(audio_sampling, "44.1kHz") == 0) {
    audio->info.audio_sampling = ST31_SAMPLING_44K;
  } else {
    logger->error("{}, invalid audio sampling {}", __func__, audio_sampling);
    return st_app_errc::JSON_NOT_VALID;
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc parse_audio_ptime(json_object* audio_obj, st_json_audio_session_t* audio) {
  const char* audio_ptime =
      json_object_get_string(st_json_object_object_get(audio_obj, "audio_ptime"));
  if (audio_ptime) {
    if (strcmp(audio_ptime, "1") == 0) {
      audio->info.audio_ptime = ST30_PTIME_1MS;
    } else if (strcmp(audio_ptime, "0.12") == 0) {
      audio->info.audio_ptime = ST30_PTIME_125US;
    } else if (strcmp(audio_ptime, "0.25") == 0) {
      audio->info.audio_ptime = ST30_PTIME_250US;
    } else if (strcmp(audio_ptime, "0.33") == 0) {
      audio->info.audio_ptime = ST30_PTIME_333US;
    } else if (strcmp(audio_ptime, "4") == 0) {
      audio->info.audio_ptime = ST30_PTIME_4MS;
    } else if (strcmp(audio_ptime, "0.08") == 0) {
      audio->info.audio_ptime = ST31_PTIME_80US;
    } else if (strcmp(audio_ptime, "1.09") == 0) {
      audio->info.audio_ptime = ST31_PTIME_1_09MS;
    } else if (strcmp(audio_ptime, "0.14") == 0) {
      audio->info.audio_ptime = ST31_PTIME_0_14MS;
    } else if (strcmp(audio_ptime, "0.09") == 0) {
      audio->info.audio_ptime = ST31_PTIME_0_09MS;
    } else {
      logger->error("{}, invalid audio ptime {}", __func__, audio_ptime);
      return st_app_errc::JSON_NOT_VALID;
    }
  } else { /* default ptime 1ms */
    audio->info.audio_ptime = ST30_PTIME_1MS;
  }
  return st_app_errc::SUCCESS;
}

/*
{
  "redundant": {
    "enable": false,
    "interface": 1,
    "ip": "239.10.1.101",
    "port": 20000
  }
}
*/
static st_app_errc parse_redundant(st_json_context *ctx, json_object *root_obj, st_json_session_base *base) {
  st_app_errc ret = st_app_errc::SUCCESS;
  json_object *r = st_json_object_object_get(root_obj, "redundant");
  base->num_inf = 1;
  if (r) {
    if (json_object_get_boolean(st_json_object_object_get(r, "enable")) &&
      ctx->interfaces.size() > 1) {
      const char *ip_str = json_object_get_string(st_json_object_object_get(r, "ip"));
      ret = parse_multicast_ip_addr_str(
        ip_str,
        base->ip[1]);
      ERRC_EXPECT_SUCCESS(ret);
      base->ip_str[1] = ip_str;
      
      ret = parse_udp_port(st_json_object_object_get(r, "port"), base->udp_port[1]);
      ERRC_EXPECT_SUCCESS(ret);
      json_object *interface_obj = st_json_object_object_get(r, "interface");
      if (!interface_obj) {
        return st_app_errc::JSON_NOT_VALID;
      }
      int inf_index = json_object_get_int(interface_obj);
      if (inf_index < 0 || inf_index >= ctx->interfaces.size()) {
        logger->error("parse_redundant invalid interface: {}", inf_index);
        return st_app_errc::JSON_NOT_VALID;
      }
      base->inf[1] = ctx->interfaces[inf_index];

      base->num_inf = 2;
    }
  }
  return st_app_errc::SUCCESS;
}

static st_app_errc st_json_parse_tx_audio(json_object* audio_obj, st_json_audio_session_t* audio) {
  if (audio_obj == NULL || audio == NULL) {
    logger->error("{}, can not parse tx audio session", __func__);
    return st_app_errc::JSON_NULL;
  }
  st_app_errc ret;

  /* parse udp port  */
  ret = parse_base_udp_port(audio_obj, &audio->base);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse payload type */
  ret = parse_base_payload_type(audio_obj, &audio->base);
  ERRC_EXPECT_SUCCESS(ret);

  parse_base_enable(audio_obj, &audio->base);

  /* parse audio type */
  /* ret = parse_audio_type(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret); */

  /* parse audio format */
  ret = parse_audio_format(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse audio channel */
  ret = parse_audio_channel(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse audio sampling */
  ret = parse_audio_sampling(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse audio packet time */
  ret = parse_audio_ptime(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse audio url */
  // ret = parse_url(audio_obj, "audio_url", audio->info.audio_url);
  // ERRC_EXPECT_SUCCESS(ret);

  return st_app_errc::SUCCESS;
}

static st_app_errc st_json_parse_rx_audio(int idx, json_object* audio_obj,
                                  st_json_audio_session_t* audio) {
  if (audio_obj == NULL || audio == NULL) {
    logger->error("{}, can not parse rx audio session", __func__);
    return st_app_errc::JSON_NULL;
  }
  st_app_errc ret;

  /* parse udp port  */
  ret = parse_base_udp_port(audio_obj, &audio->base);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse payload type */
  ret = parse_base_payload_type(audio_obj, &audio->base);
  ERRC_EXPECT_SUCCESS(ret);

  parse_base_enable(audio_obj, &audio->base);

  /* parse audio type */
  /* ret = parse_audio_type(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret); */

  /* parse audio format */
  ret = parse_audio_format(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse audio channel */
  ret = parse_audio_channel(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse audio sampling */
  ret = parse_audio_sampling(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse audio packet time */
  ret = parse_audio_ptime(audio_obj, audio);
  ERRC_EXPECT_SUCCESS(ret);

  /* parse audio url */
  // ret = parse_url(audio_obj, "audio_url", audio->info.audio_url);
  // if (ERRC_FAILED(ret)) {
  //   logger->error("{}, no reference file", __func__);
  // }

  return st_app_errc::SUCCESS;
}


static st_app_errc parse_session_num(json_object* group, const char* name, int &num) {
  num = 0;
  json_object* session_array = st_json_object_object_get(group, name);
  if (session_array != NULL && json_object_get_type(session_array) == json_type_array) {
    num = json_object_array_length(session_array);
  }
  return st_app_errc::SUCCESS;
}


namespace {

class JsonObjectDeleter {
public:
  void operator()(json_object *o) {
    json_object_put(o);
  }
};

class JsonTokenerDeleter {
public:
  void operator()(json_tokener *tokener) {
    if (tokener) {
      json_tokener_free(tokener);
    }
  }
};

std::unique_ptr<json_object, JsonObjectDeleter> st_app_parse_json_get_object(const std::string &s) {
  std::unique_ptr<json_object, JsonObjectDeleter> root;
  auto tokener = std::unique_ptr<json_tokener, JsonTokenerDeleter>(json_tokener_new());
  if (tokener) {
    json_object *o = json_tokener_parse_ex(tokener.get(), s.c_str(), s.size());
    if (o) {
      root = std::unique_ptr<json_object, JsonObjectDeleter>(o);
    }
  }
  return root;
}

std::string safe_get_string(json_object *o, const char *name) {
  const char *v = json_object_get_string(st_json_object_object_get(o, name));
  if (v) {
    return v;
  } else {
    return "";
  }
}

st_app_errc json_put_string(json_object *o, const std::string &key, const std::string &value) {
  json_object *val = json_object_new_string(value.c_str());
  if (json_object_object_add(o, key.c_str(), val)) {
    json_object_put(val);
  }
  return st_app_errc::SUCCESS;
}

st_app_errc st_app_parse_json_sch(st_json_context_t* ctx, json_object *root_object) {
  /* parse quota for system */
  json_object* sch_quota_object =
      st_json_object_object_get(root_object, "sch_session_quota");
  if (sch_quota_object != NULL) {
    int sch_quota = json_object_get_int(sch_quota_object);
    if (sch_quota <= 0) {
      logger->error("{}, invalid quota number", __func__);
      return st_app_errc::JSON_NOT_VALID;
    }
    ctx->sch_quota = sch_quota;
  }
  return st_app_errc::SUCCESS;
}

st_app_errc st_app_parse_json_interfaces(st_json_context_t* ctx, json_object *root_object) {
  st_app_errc ret = st_app_errc::SUCCESS;
  /* parse interfaces for system */
  json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
  if (interfaces_array == NULL ||
      json_object_get_type(interfaces_array) != json_type_array) {
    logger->error("{}, can not parse interfaces", __func__);
    return st_app_errc::JSON_PARSE_FAIL;
  }
  int num_interfaces = json_object_array_length(interfaces_array);
  ctx->interfaces.resize(num_interfaces);
  for (int i = 0; i < num_interfaces; ++i) {
    ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
                                   &ctx->interfaces[i]);
    if (ERRC_FAILED(ret)) return ret;
  }
  return ret;
}

st_app_errc st_app_parse_json_tx_sessions(st_json_context_t* ctx, json_object *root_object) {
  st_app_errc ret = st_app_errc::SUCCESS;
  int num_interfaces = ctx->interfaces.size();
  /* parse tx sessions  */
  json_object* tx_group_array = st_json_object_object_get(root_object, "tx_sessions");
  if (tx_group_array != NULL && json_object_get_type(tx_group_array) == json_type_array) {
    /* parse session numbers for array allocation */
    for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
      json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
      if (tx_group == NULL) {
        logger->error("{}, can not parse tx session group", __func__);
        return st_app_errc::JSON_PARSE_FAIL;
      }
      // parse tx source config
      json_object* source = st_json_object_object_get(tx_group, "source");
      auto &tx_source = ctx->tx_sources.emplace_back();
      tx_source.type = safe_get_string(source, "type");
      tx_source.device_id = json_object_get_int(st_json_object_object_get(source, "device_id"));
      if (!tx_source.device_id) {
        return st_app_errc::DECKLINK_DEVICE_ID_REQUIRED;
      }
      tx_source.video_format = safe_get_string(source, "video_format");
      tx_source.pixel_format = safe_get_string(source, "pixel_format");
      
      tx_source.id = safe_get_string(tx_group, "id");
      if (tx_source.id.empty()) {
        tx_source.id = seeder::core::generate_uuid();
      }
    }

    std::unordered_map<std::string, int> source_id_map;
    for (auto &tx_source : ctx->tx_sources) {
      if (tx_source.id.empty()) {
        logger->error("empty tx_source.id {}", tx_source.id);
        return st_app_errc::JSON_NOT_VALID;
      }
      if (++source_id_map[tx_source.id] > 1) {
        logger->error("duplicated tx_source.id {}", tx_source.id);
        return st_app_errc::JSON_NOT_VALID;
      }
    }

    for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
      json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
      if (tx_group == NULL) {
        logger->error("{}, can not parse tx session group", __func__);
        return st_app_errc::JSON_PARSE_FAIL;
      }

      auto &tx_source = ctx->tx_sources[i];
      json_put_string(tx_group, "id", tx_source.id);

      /* parse destination ip */
      json_object* dip_p = NULL;
      json_object* dip_array = st_json_object_object_get(tx_group, "dip");
      if (dip_array != NULL && json_object_get_type(dip_array) == json_type_array) {
        int len = json_object_array_length(dip_array);
        if (len < 1 || len > MTL_PORT_MAX) {
          logger->error("{}, wrong dip number", __func__);
          return st_app_errc::JSON_NOT_VALID;
        }
        dip_p = json_object_array_get_idx(dip_array, 0);
      }

      /* parse interface */
      int inf_p = 0;
      json_object* interface_array = st_json_object_object_get(tx_group, "interface");
      if (interface_array != NULL &&
          json_object_get_type(interface_array) == json_type_array) {
        inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
        if (inf_p < 0 || inf_p > num_interfaces) {
          logger->error("{}, wrong interface index", __func__);
          return st_app_errc::JSON_NOT_VALID;
        }
      } else {
        logger->error("{}, can not parse interface_array", __func__);
        return st_app_errc::JSON_PARSE_FAIL;
      }

      /* parse outputsdi */
      json_object* outputsdi_array = st_json_object_object_get(tx_group, "outputsdi");
      if(outputsdi_array != NULL && json_object_get_type(outputsdi_array) == json_type_array)
      {
        for(int i = 0;i < json_object_array_length(outputsdi_array); ++i)
        {
          json_object* outputsdi_session = json_object_array_get_idx(outputsdi_array, i);
          st_app_tx_output_sdi &output_sdi = ctx->tx_outputsdi.emplace_back();
          ret = st_json_parse_tx_outputsdi(outputsdi_session, &output_sdi);
          ERRC_EXPECT_SUCCESS(ret);
          output_sdi.id = tx_source.id;
        }
      }

      /* parse tx video sessions */
      json_object* video_array = st_json_object_object_get(tx_group, "video");
      if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(video_array); ++j) {
          st_json_video_session &tx_video_session = ctx->tx_video_sessions.emplace_back();
          json_object* video_session = json_object_array_get_idx(video_array, j);
          // 取video对象的dip，没有的话用外层的dip
          json_object* video_dip_p = NULL;
          json_object* video_dip_array = st_json_object_object_get(video_session, "dip");
          if (video_dip_array != NULL && json_object_get_type(video_dip_array) == json_type_array) {
            int len = json_object_array_length(video_dip_array);
            if (len < 1 || len > MTL_PORT_MAX) {
              logger->error("{}, wrong dip number", __func__);
              return st_app_errc::JSON_NOT_VALID;
            }
            video_dip_p = json_object_array_get_idx(video_dip_array, 0);
          } else {
            video_dip_p = dip_p;
          }

          ret = parse_multicast_ip_addr_str(json_object_get_string(video_dip_p), tx_video_session.base.ip[0]);
          ERRC_EXPECT_SUCCESS(ret);
          tx_video_session.base.ip_str[0] = json_object_get_string(video_dip_p);

          tx_video_session.base.inf[0] = ctx->interfaces[inf_p];
          
          ret = parse_redundant(ctx, video_session, &tx_video_session.base);
          ERRC_EXPECT_SUCCESS(ret);

          ret = st_json_parse_tx_video(video_session, &tx_video_session);
          ERRC_EXPECT_SUCCESS(ret);

          // video source handle id
          tx_video_session.base.id = tx_source.id;

          const st_video_fmt_desc *fmt_desc = get_video_fmt_from_core_fmt_name(tx_source.video_format);
          if (!fmt_desc) {
            logger->error("tx_sessions.video.video_format={} get_video_fmt_from_core_fmt_name failed", tx_source.video_format);
            return st_app_errc::JSON_NOT_VALID;
          }
          tx_video_session.info.video_format = fmt_desc->fmt;
        }
      }

      /* parse tx audio sessions */
      json_object* audio_array = st_json_object_object_get(tx_group, "audio");
      if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(audio_array); ++j) {
          st_json_audio_session_t &tx_audio_session = ctx->tx_audio_sessions.emplace_back();
          json_object* audio_session = json_object_array_get_idx(audio_array, j);
          // 取audio对象的dip，没有的话用外层的dip
          json_object* audio_dip_p = NULL;
          json_object* audio_dip_array = st_json_object_object_get(audio_session, "dip");
          if (audio_dip_array != NULL && json_object_get_type(audio_dip_array) == json_type_array) {
            int len = json_object_array_length(audio_dip_array);
            if (len < 1 || len > MTL_PORT_MAX) {
              logger->error("{}, wrong dip number", __func__);
              return st_app_errc::JSON_NOT_VALID;
            }
            audio_dip_p = json_object_array_get_idx(audio_dip_array, 0);
          } else {
            audio_dip_p = dip_p;
          }
          ret = parse_multicast_ip_addr_str(json_object_get_string(audio_dip_p), tx_audio_session.base.ip[0]);
          ERRC_EXPECT_SUCCESS(ret);
          tx_audio_session.base.ip_str[0] = json_object_get_string(audio_dip_p);

          tx_audio_session.base.inf[0] = ctx->interfaces[inf_p];
          ret = parse_redundant(ctx, audio_session, &tx_audio_session.base);
          ERRC_EXPECT_SUCCESS(ret);
          
          ret = st_json_parse_tx_audio(audio_session, &tx_audio_session);
          ERRC_EXPECT_SUCCESS(ret);

          // audio source handle id
          tx_audio_session.base.id = tx_source.id;
        }
      }
    }
  }

  return ret;
}

namespace {

template<class K, class V>
Json::Value make_fmt_item(K &&name, V &&value) {
  Json::Value item;
  item["name"] = std::forward<K>(name);
  item["value"] = std::forward<V>(value);
  return item;
}

struct st_app_json_fmt_item {
  std::string name;
  Json::Value value;

  template<class K, class V>
  st_app_json_fmt_item(K &&_name, V &&_value):
    name(std::forward<K>(_name)), value(std::forward<V>(_value))
  {}

  Json::Value to_json() const {
    Json::Value item;
    item["name"] = name;
    item["value"] = value;
    return item;
  }
};

struct st_app_json_fmt_group {
  st_app_json_fmt_group(const std::string &_name, std::initializer_list<st_app_json_fmt_item> _items):
    name(_name), items(_items)
  {}

  std::string name;
  std::vector<st_app_json_fmt_item> items;

  static const std::vector<st_app_json_fmt_group> & get_common_groups() {
    static std::vector<st_app_json_fmt_group> groups = {
      {
        "video.replicas", // TODO 移除
        {
          {"1", 1}
        }
      },
      {
        "video.type", // TODO 移除
        {
          {"frame", "frame"},
          // {"rtp", "rtp"},
          // {"slice", "slice"},
        }
      },
      {
        "video.pacing",
        {
          {"gap", "gap"},
          {"linear", "linear"}
        }
      },
      {
        "video.packing",
        {
          {"BPM", "BPM"},
          {"GPM SL", "GPM_SL"},
          {"GPM", "GPM"},
        }
      },
      {
        "video.tr_offset", // TODO 移除
        {
          {"default", "default"},
          {"none", "none"}
        }
      },
      {
        "video.pg_format", // TODO 移除
        {
          {"YUV 422 10bit", "YUV_422_10bit"},
          // {"YUV 422 8bit", "YUV_422_8bit"},
          // {"YUV 422 12bit", "YUV_422_12bit"},
          // {"YUV 422 16bit", "YUV_422_16bit"},
          // {"YUV 420 8bit", "YUV_420_8bit"},
          // {"YUV 420 10bit", "YUV_420_10bit"},
          // {"YUV 420 12bit", "YUV_420_12bit"},
          // {"RGB 8bit", "RGB_8bit"},
          // {"RGB 10bit", "RGB_10bit"},
          // {"RGB 12bit", "RGB_12bit"},
          // {"RGB 16bit", "RGB_16bit"},
        }
      },
      {
        "audio.replicas", // TODO 移除
        {
          {"1", 1}
        }
      },
      {
        "audio.type", // TODO 移除
        {
          {"frame", "frame"},
          // {"rtp", "rtp"}
        }
      },
      {
        "audio.audio_format",
        {
          {"16", "PCM16"},
          // {"PCM 8", "PCM8"},
          {"24", "PCM24"},
          // {"AM 824", "AM824"}
        }
      },
      {
        "audio.audio_channel",
        {
          {"ST", "ST"},
          // {"M", "M"},
          // {"51", "51"},
          // {"71", "71"}
        }
      },
      {
        "audio.audio_sampling",
        {
            {"48kHz", "48kHz"},
            // {"96kHz", "96kHz"},
            // {"44.1kHz", "44.1kHz"}
        }
      },
      {
        "audio.audio_ptime",
        {
          {"1", "1"},
          // {"0.12", "0.12"},
          {"0.25", "0.25"},
          // {"0.33", "0.33"},
          // {"4", "4"},
          // {"0.08", "0.08"},
          // {"1.09", "1.09"},
          // {"0.14", "0.14"},
          // {"0.09", "0.09"}
        }
      },
    };
    return groups;
  }
};

void fmts_set_decklink_devices(st_json_context_t* ctx, Json::Value &root) {
  if (ctx->json_root["decklink"].isObject() && ctx->json_root["decklink"]["devices"].isArray()) {
    const auto &devices = ctx->json_root["decklink"]["devices"];
    for (Json::Value::ArrayIndex i = 0; i < devices.size(); ++i) {
      auto &device = devices[i];
      auto device_name = device["display_name"].asString();
      auto device_id = std::to_string(i + 1);
      root["tx_sessions.source.device_id"].append(make_fmt_item(device_name, device_id));
      root["rx_sessions.output.device_id"].append(make_fmt_item(device_name, device_id));
      root["tx_sessions.outputsdi.device_id"].append(make_fmt_item(device_name, device_id));
    }
  } else {
    auto &decklink_manager = seeder::decklink::device_manager::instance();
    int decklink_device_count = decklink_manager.get_device_status().size();
    for (int i = 0; i < decklink_device_count; ++i) {
      auto device_name = "SDI " + std::to_string(i + 1);
      root["tx_sessions.source.device_id"].append(make_fmt_item(device_name, std::to_string(i + 1)));
      root["rx_sessions.output.device_id"].append(make_fmt_item(device_name, std::to_string(i + 1)));
      root["tx_sessions.outputsdi.device_id"].append(make_fmt_item(device_name, std::to_string(i + 1)));
    }
  }
}

} // namespace


st_app_errc st_app_parse_json_rx_sessions(st_json_context_t* ctx, json_object *root_object) {
  st_app_errc ret = st_app_errc::SUCCESS;
  int num_interfaces = ctx->interfaces.size();

  /* parse rx sessions */
  json_object* rx_group_array = st_json_object_object_get(root_object, "rx_sessions");
  if (rx_group_array != NULL && json_object_get_type(rx_group_array) == json_type_array) {
    /* parse session numbers for array allocation */
    for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
      json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
      if (rx_group == NULL) {
        logger->error("{}, can not parse rx session group", __func__);
        return st_app_errc::JSON_PARSE_FAIL;
      }

      // parse rx output config
      json_object* output = st_json_object_object_get(rx_group, "output");
      auto &rx_output = ctx->rx_output.emplace_back();
      rx_output.type = safe_get_string(output, "type");
      rx_output.device_id = json_object_get_int(st_json_object_object_get(output, "device_id"));
      if (!rx_output.device_id) {
        return st_app_errc::DECKLINK_DEVICE_ID_REQUIRED;
      }
      rx_output.video_format = safe_get_string(output, "video_format");
      rx_output.pixel_format = safe_get_string(output, "pixel_format");

      rx_output.id = safe_get_string(rx_group, "id");
      if (rx_output.id.empty()) {
        rx_output.id = seeder::core::generate_uuid();
      }
    }

    std::unordered_map<std::string, int> output_id_map;
    for (auto &rx_output : ctx->rx_output) {
      if (rx_output.id.empty()) {
        logger->error("empty rx_output.id");
        return st_app_errc::JSON_NOT_VALID;
      }
      if (++output_id_map[rx_output.id] > 1) {
        logger->error("duplicated rx_output.id {}", rx_output.id);
        return st_app_errc::JSON_NOT_VALID;
      }
    }

    for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
      json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
      if (rx_group == NULL) {
        logger->error("{}, can not parse rx session group", __func__);
        return st_app_errc::JSON_PARSE_FAIL;
      }

      auto &rx_output = ctx->rx_output[i];
      json_put_string(rx_group, "id", rx_output.id);

      /* parse receiving ip */
      json_object* ip_p = NULL;
      json_object* ip_array = st_json_object_object_get(rx_group, "ip");
      if (ip_array != NULL && json_object_get_type(ip_array) == json_type_array) {
        int len = json_object_array_length(ip_array);
        if (len < 1 || len > MTL_PORT_MAX) {
          logger->error("{}, wrong dip number", __func__);
          return st_app_errc::JSON_NOT_VALID;
        }
        ip_p = json_object_array_get_idx(ip_array, 0);
      }

      /* parse interface */
      int inf_p = 0;
      json_object* interface_array = st_json_object_object_get(rx_group, "interface");
      if (interface_array != NULL && json_object_get_type(interface_array) == json_type_array) {
        inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
        if (inf_p < 0 || inf_p > num_interfaces) {
          logger->error("{}, wrong interface index", __func__);
          return st_app_errc::JSON_NOT_VALID;
        }
      } else {
        logger->error("{}, can not parse interface_array", __func__);
        return st_app_errc::JSON_PARSE_FAIL;
      }

      /* parse rx video sessions */
      json_object* video_array = st_json_object_object_get(rx_group, "video");
      if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(video_array); ++j) {
          st_json_video_session &rx_video_session = ctx->rx_video_sessions.emplace_back();

          json_object* video_session = json_object_array_get_idx(video_array, j);
          // 取video对象的ip，没有的话用外层的ip
          json_object* video_sip_p = NULL;
          json_object* video_sip_array = st_json_object_object_get(video_session, "ip");
          if (video_sip_array != NULL && json_object_get_type(video_sip_array) == json_type_array) {
            int len = json_object_array_length(video_sip_array);
            if (len < 1 || len > MTL_PORT_MAX) {
              logger->error("{}, wrong dip number", __func__);
              return st_app_errc::JSON_NOT_VALID;
            }
            video_sip_p = json_object_array_get_idx(video_sip_array, 0);
          } else {
            video_sip_p = ip_p;
          }
          
          ret = parse_multicast_ip_addr_str(json_object_get_string(video_sip_p), rx_video_session.base.ip[0]);
          ERRC_EXPECT_SUCCESS(ret);
          rx_video_session.base.ip_str[0] = json_object_get_string(video_sip_p);

          rx_video_session.base.inf[0] = ctx->interfaces[inf_p];

          ret = parse_redundant(ctx, video_session, &rx_video_session.base);
          ERRC_EXPECT_SUCCESS(ret);

          ret = st_json_parse_rx_video(video_session, &rx_video_session);
          ERRC_EXPECT_SUCCESS(ret);

          // video output handle id
          rx_video_session.base.id = rx_output.id;

          const st_video_fmt_desc *fmt_desc = get_video_fmt_from_core_fmt_name(rx_output.video_format);
          if (!fmt_desc) {
            logger->error("rx_sessions.video.video_format={} get_video_fmt_from_core_fmt_name failed", rx_output.video_format);
            return st_app_errc::JSON_NOT_VALID;
          }
          rx_video_session.info.video_format = fmt_desc->fmt;
        }
      }

      /* parse rx audio sessions */
      json_object* audio_array = st_json_object_object_get(rx_group, "audio");
      if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(audio_array); ++j) {
          st_json_audio_session &rx_audio_session = ctx->rx_audio_sessions.emplace_back();

          json_object* audio_session = json_object_array_get_idx(audio_array, j);
          // 取audio对象的dip，没有的话用外层的dip
          json_object* audio_sip_p = NULL;
          json_object* audio_sip_array = st_json_object_object_get(audio_session, "ip");
          if (audio_sip_array != NULL && json_object_get_type(audio_sip_array) == json_type_array) {
            int len = json_object_array_length(audio_sip_array);
            if (len < 1 || len > MTL_PORT_MAX) {
              logger->error("{}, wrong dip number", __func__);
              return st_app_errc::JSON_NOT_VALID;
            }
            audio_sip_p = json_object_array_get_idx(audio_sip_array, 0);
          } else {
            audio_sip_p = ip_p;
          }

          ret = parse_multicast_ip_addr_str(json_object_get_string(audio_sip_p), rx_audio_session.base.ip[0]);
          ERRC_EXPECT_SUCCESS(ret);
          rx_audio_session.base.ip_str[0] = json_object_get_string(audio_sip_p);

          rx_audio_session.base.inf[0] = ctx->interfaces[inf_p];

          ret = parse_redundant(ctx, audio_session, &rx_audio_session.base);
          ERRC_EXPECT_SUCCESS(ret);

          ret = st_json_parse_rx_audio(0, audio_session,
                                        &rx_audio_session);
          ERRC_EXPECT_SUCCESS(ret);

          // video output handle id
          rx_audio_session.base.id = rx_output.id;
        }
      }

    }
  }

  return ret;
}

st_app_errc st_app_parse_json(st_json_context_t* ctx, json_object *root_object) {
  st_app_errc ret = st_app_errc::SUCCESS;
  ret = st_app_parse_json_sch(ctx, root_object);
  ERRC_EXPECT_SUCCESS(ret);
  ret = st_app_parse_json_interfaces(ctx, root_object);
  ERRC_EXPECT_SUCCESS(ret);
  ret = st_app_parse_json_tx_sessions(ctx, root_object);
  ERRC_EXPECT_SUCCESS(ret);
  ret = st_app_parse_json_rx_sessions(ctx, root_object);
  ERRC_EXPECT_SUCCESS(ret);
  return ret;
}

std::unique_ptr<st_json_context_t> prepare_new_json_context(st_json_context_t* ctx) {
  auto new_ctx = std::unique_ptr<st_json_context_t>(new st_json_context_t {});
  new_ctx->interfaces = ctx->interfaces;
  new_ctx->sch_quota = ctx->sch_quota;
  return new_ctx;
}

void set_json_root(st_json_context_t* ctx, json_object *object) {
  const char *json_str = json_object_to_json_string(object);
  Json::Reader reader;
  reader.parse(json_str, ctx->json_root);
  for(auto &s :ctx->json_root["tx_sessions"]) {
    if (!s["outputsdi"].isArray()) {
      //增加一个空的数组类型
      Json::Value outputsdi;
      outputsdi["enable"] = false;
      outputsdi["device_id"] = Json::nullValue;
      s["outputsdi"].append(outputsdi);

      //s["outputsdi"] = Json::Value(Json::ValueType::arrayValue);
    }
  }
}

} // namespace

std::error_code st_app_parse_json(st_json_context_t* ctx, const char* filename) {
  //logger->info("{}, using json-c version: {}", __func__, json_c_version());

  std::unique_ptr<json_object, JsonObjectDeleter> root_object { json_object_from_file(filename) };
  if (root_object == nullptr) {
    logger->error("{}, can not parse json file {}, please check the format", __func__, filename);
    return st_app_errc::JSON_PARSE_FAIL;
  }
  const char *json_content = json_object_to_json_string(root_object.get());
  logger->debug("st_app_parse_json(): {}", json_content);
  st_app_errc ret = st_app_parse_json(ctx, root_object.get());
  set_json_root(ctx, root_object.get());
  return ret;
}

enum st_fps st_app_get_fps(enum video_format fmt) {
  int i;

  for (i = 0; i < ARRAY_SIZE(st_video_fmt_descs); i++) {
    if (fmt == st_video_fmt_descs[i].fmt) {
      return st_video_fmt_descs[i].fps;
    }
  }

  logger->error("{}, invalid fmt {}", __func__, fmt);
  return ST_FPS_P59_94;
}

uint32_t st_app_get_width(enum video_format fmt) {
  int i;

  for (i = 0; i < ARRAY_SIZE(st_video_fmt_descs); i++) {
    if (fmt == st_video_fmt_descs[i].fmt) {
      return st_video_fmt_descs[i].width;
    }
  }

  logger->error("{}, invalid fmt {}", __func__, fmt);
  return 1920;
}

uint32_t st_app_get_height(enum video_format fmt) {
  int i;

  for (i = 0; i < ARRAY_SIZE(st_video_fmt_descs); i++) {
    if (fmt == st_video_fmt_descs[i].fmt) {
      return st_video_fmt_descs[i].height;
    }
  }

  logger->error("{}, invalid fmt {}", __func__, fmt);
  return 1080;
}

bool st_app_get_interlaced(enum video_format fmt) {
  switch (fmt) {
    case VIDEO_FORMAT_480I_59FPS:
    case VIDEO_FORMAT_576I_50FPS:
    case VIDEO_FORMAT_1080I_59FPS:
    case VIDEO_FORMAT_1080I_50FPS:
      return true;
    default:
      return false;
  }
}

static st_app_errc st_app_parse_json_check_new_context(st_json_context_t *ctx) {
  st_app_errc ret = st_app_errc::SUCCESS;
  if (!ctx->tx_sources.empty()) {
    if (ctx->tx_video_sessions.empty()) {
      return st_app_errc::VIDEO_SESSION_REQUIRED;
    }
    if (ctx->tx_audio_sessions.empty()) {
      return st_app_errc::AUDIO_SESSION_REQUIRED;
    }
    if (ctx->tx_video_sessions.size() != ctx->tx_audio_sessions.size()) {
      if (ctx->tx_video_sessions.size() > ctx->tx_audio_sessions.size()) {
        return st_app_errc::AUDIO_SESSION_REQUIRED;
      } else {
        return st_app_errc::VIDEO_SESSION_REQUIRED;
      }
    }
  }
  if (!ctx->rx_output.empty()) {
    if (ctx->rx_video_sessions.empty()) {
      return st_app_errc::VIDEO_SESSION_REQUIRED;
    }
    if (ctx->rx_audio_sessions.empty()) {
      return st_app_errc::VIDEO_SESSION_REQUIRED;
    }
    if (ctx->rx_video_sessions.size() != ctx->rx_audio_sessions.size()) {
      if (ctx->rx_video_sessions.size() > ctx->rx_audio_sessions.size()) {
        return st_app_errc::AUDIO_SESSION_REQUIRED;
      } else {
        return st_app_errc::VIDEO_SESSION_REQUIRED;
      }
    }
  }
  return ret;
}


std::error_code st_app_parse_json_add(st_json_context_t* ctx, const std::string &json, std::unique_ptr<st_json_context_t> &new_ctx) {
  new_ctx = prepare_new_json_context(ctx);
  auto root = st_app_parse_json_get_object(json);
  st_app_errc ret = st_app_errc::SUCCESS;
  ret = st_app_parse_json_tx_sessions(new_ctx.get(), root.get());
  ERRC_EXPECT_SUCCESS(ret);
  ret = st_app_parse_json_rx_sessions(new_ctx.get(), root.get());
  ERRC_EXPECT_SUCCESS(ret);
  set_json_root(new_ctx.get(), root.get());
  ret = st_app_parse_json_check_new_context(new_ctx.get());
  return ret;
}


std::error_code st_app_parse_json_update(st_json_context_t* ctx, const std::string &json, std::unique_ptr<st_json_context_t> &new_ctx) {
  new_ctx = prepare_new_json_context(ctx);
  auto root = st_app_parse_json_get_object(json);
  st_app_errc ret = st_app_errc::SUCCESS;
  ret = st_app_parse_json_tx_sessions(new_ctx.get(), root.get());
  ERRC_EXPECT_SUCCESS(ret);
  ret = st_app_parse_json_rx_sessions(new_ctx.get(), root.get());
  ERRC_EXPECT_SUCCESS(ret);
  set_json_root(new_ctx.get(), root.get());
  ret = st_app_parse_json_check_new_context(new_ctx.get());
  return ret;
}


Json::Value st_app_get_fmts(st_json_context_t* ctx) {
  Json::Value root;

  for (int i = 0; i < ctx->interfaces.size(); ++i) {
    auto inf_name = "WAN" + std::to_string(i + 1);
    if (!ctx->interfaces[i].ip_addr_str.empty()) {
      inf_name.append(" (").append(ctx->interfaces[i].ip_addr_str).append(")");
    }
    auto item = make_fmt_item(inf_name, i);
    root["tx_sessions.interface"].append(item);
    root["rx_sessions.interface"].append(item);
    root["tx_sessions.video.redundant.interface"].append(item);
    root["tx_sessions.audio.redundant.interface"].append(item);
    root["rx_sessions.video.redundant.interface"].append(item);
    root["rx_sessions.audio.redundant.interface"].append(item);
  }

  root["tx_sessions.source.type"].append(make_fmt_item("decklink", "decklink"));
  root["rx_sessions.output.type"].append(make_fmt_item("decklink", "decklink"));
  // root["rx_sessions.output.type"].append(make_fmt_item("decklink", "decklink_async"));
  auto &tx_source_video_format = root["tx_sessions.source.video_format"];
  auto &rx_output_video_format = root["rx_sessions.output.video_format"];
  for (auto &f : st_app_get_video_fmts()) {
    auto &core_fmt = seeder::core::video_format_desc::get(f.core_fmt);
    tx_source_video_format.append(make_fmt_item(core_fmt.display_name, core_fmt.name));
    rx_output_video_format.append(make_fmt_item(core_fmt.display_name, core_fmt.name));
  }

  const std::vector<std::string> session_types {
    "tx_sessions",
    "rx_sessions"
  };

  for (auto &sessions : session_types) {
    // auto video_video_format_key = sessions + ".video.video_format";
    // for (auto &f : st_app_get_video_fmts()) {
    //   root[video_video_format_key].append(make_fmt_item(f.name, f.name));
    // }

    for (auto &group : st_app_json_fmt_group::get_common_groups()) {
      auto group_key = sessions + "." + group.name;
      for (auto &group_item : group.items) {
        root[group_key].append(group_item.to_json());
      }
    }
  }

  fmts_set_decklink_devices(ctx, root);

  return root;
}

st_json_context::~st_json_context() {}