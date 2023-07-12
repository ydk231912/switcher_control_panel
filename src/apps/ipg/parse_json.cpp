/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */

#include "parse_json.h"

#include "app_base.h"
#include "core/util/logger.h"
#include "core/util/uuid.h"
#include "core/util/fs.h"
#include <json/reader.h>
#include <json/value.h>
#include <json_object.h>
#include <json_tokener.h>
#include <memory>
#include <unordered_map>
#include <json-c/json.h>
#include "core/video_format.h"


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

static const struct st_video_fmt_desc st_video_fmt_descs[] = {
    {
        .fmt = VIDEO_FORMAT_480I_59FPS,
        .name = "i480i59",
        .width = 720,
        .height = 480,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_576I_50FPS,
        .name = "i576i50",
        .width = 720,
        .height = 576,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_720P_119FPS,
        .name = "i720p119",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P119_88,
    },
    {
        .fmt = VIDEO_FORMAT_720P_59FPS,
        .name = "i720p59",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_720P_50FPS,
        .name = "i720p50",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_720P_29FPS,
        .name = "i720p29",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P29_97,
    },
    {
        .fmt = VIDEO_FORMAT_720P_25FPS,
        .name = "i720p25",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P25,
    },
    {
        .fmt = VIDEO_FORMAT_720P_60FPS,
        .name = "i720p60",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P60,
    },
    {
        .fmt = VIDEO_FORMAT_720P_30FPS,
        .name = "i720p30",
        .width = 1280,
        .height = 720,
        .fps = ST_FPS_P30,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_119FPS,
        .name = "i1080p119",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P119_88,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_59FPS,
        .name = "i1080p59",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_50FPS,
        .name = "i1080p50",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_29FPS,
        .name = "i1080p29",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P29_97,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_25FPS,
        .name = "i1080p25",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P25,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_60FPS,
        .name = "i1080p60",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P60,
    },
    {
        .fmt = VIDEO_FORMAT_1080P_30FPS,
        .name = "i1080p30",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P30,
    },
    {
        .fmt = VIDEO_FORMAT_1080I_59FPS,
        .name = "i1080i59",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_1080I_50FPS,
        .name = "i1080i50",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_119FPS,
        .name = "i2160p119",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P119_88,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_59FPS,
        .name = "i2160p59",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_50FPS,
        .name = "i2160p50",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_29FPS,
        .name = "i2160p29",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P29_97,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_25FPS,
        .name = "i2160p25",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P25,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_60FPS,
        .name = "i2160p60",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P60,
    },
    {
        .fmt = VIDEO_FORMAT_2160P_30FPS,
        .name = "i2160p30",
        .width = 3840,
        .height = 2160,
        .fps = ST_FPS_P30,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_119FPS,
        .name = "i4320p119",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P119_88,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_59FPS,
        .name = "i4320p59",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P59_94,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_50FPS,
        .name = "i4320p50",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P50,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_29FPS,
        .name = "i4320p29",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P29_97,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_25FPS,
        .name = "i4320p25",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P25,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_60FPS,
        .name = "i4320p60",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P60,
    },
    {
        .fmt = VIDEO_FORMAT_4320P_30FPS,
        .name = "i4320p30",
        .width = 7680,
        .height = 4320,
        .fps = ST_FPS_P30,
    },
    {
        .fmt = VIDEO_FORMAT_AUTO,
        .name = "auto",
        .width = 1920,
        .height = 1080,
        .fps = ST_FPS_P59_94,
    },
};

#define VNAME(name) (#name)

#define REQUIRED_ITEM(string)                                 \
  do {                                                        \
    if (string == NULL) {                                     \
      logger->error("{}, can not parse {}", __func__, VNAME(string)); \
      return -ST_JSON_PARSE_FAIL;                             \
    }                                                         \
  } while (0)

/* 7 bits payload type define in RFC3550 */
static inline bool st_json_is_valid_payload_type(int payload_type) {
  if (payload_type > 0 && payload_type < 0x7F)
    return true;
  else
    return false;
}

static int st_json_parse_interfaces(json_object* interface_obj,
                                    st_json_interface_t* interface) {
  if (interface_obj == NULL || interface == NULL) {
    logger->error("{}, can not parse interfaces!", __func__);
    return -ST_JSON_NULL;
  }

  const char* name =
      json_object_get_string(st_json_object_object_get(interface_obj, "name"));
  REQUIRED_ITEM(name);
  snprintf(interface->name, sizeof(interface->name), "%s", name);

  const char* ip = json_object_get_string(st_json_object_object_get(interface_obj, "ip"));
  if (ip) inet_pton(AF_INET, ip, interface->ip_addr);

  return ST_JSON_SUCCESS;
}

static int parse_base_udp_port(json_object* obj, st_json_session_base_t* base, int idx) {
  int start_port = json_object_get_int(st_json_object_object_get(obj, "start_port"));
  if (start_port <= 0 || start_port > 65535) {
    logger->error("{}, invalid start port {}", __func__, start_port);
    return -ST_JSON_NOT_VALID;
  }
  base->udp_port = start_port + idx;

  return ST_JSON_SUCCESS;
}

static int parse_base_payload_type(json_object* obj, st_json_session_base_t* base) {
  json_object* payload_type_object = st_json_object_object_get(obj, "payload_type");
  if (payload_type_object) {
    base->payload_type = json_object_get_int(payload_type_object);
    if (!st_json_is_valid_payload_type(base->payload_type)) {
      logger->error("{}, invalid payload type {}", __func__, base->payload_type);
      return -ST_JSON_NOT_VALID;
    }
  } else {
    return -ST_JSON_NULL;
  }

  return ST_JSON_SUCCESS;
}

static int parse_video_type(json_object* video_obj, st_json_video_session_t* video) {
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
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_video_pacing(json_object* video_obj, st_json_video_session_t* video) {
  const char* pacing =
      json_object_get_string(st_json_object_object_get(video_obj, "pacing"));
  REQUIRED_ITEM(pacing);
  if (strcmp(pacing, "gap") == 0) {
    video->info.pacing = PACING_GAP;
  } else if (strcmp(pacing, "linear") == 0) {
    video->info.pacing = PACING_LINEAR;
  } else {
    logger->error("{}, invalid video pacing {}", __func__, pacing);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_video_packing(json_object* video_obj, st_json_video_session_t* video) {
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
      return -ST_JSON_NOT_VALID;
    }
  } else { /* default set to bpm */
    video->info.packing = ST20_PACKING_BPM;
  }
  return ST_JSON_SUCCESS;
}

static int parse_video_tr_offset(json_object* video_obj, st_json_video_session_t* video) {
  const char* tr_offset =
      json_object_get_string(st_json_object_object_get(video_obj, "tr_offset"));
  REQUIRED_ITEM(tr_offset);
  if (strcmp(tr_offset, "default") == 0) {
    video->info.tr_offset = TR_OFFSET_DEFAULT;
  } else if (strcmp(tr_offset, "none") == 0) {
    video->info.tr_offset = TR_OFFSET_NONE;
  } else {
    logger->error("{}, invalid video tr_offset {}", __func__, tr_offset);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_video_format(json_object* video_obj, st_json_video_session_t* video) {
  const char* video_format =
      json_object_get_string(st_json_object_object_get(video_obj, "video_format"));
  REQUIRED_ITEM(video_format);
  int i;
  for (i = 0; i < ARRAY_SIZE(st_video_fmt_descs); i++) {
    if (strcmp(video_format, st_video_fmt_descs[i].name) == 0) {
      video->info.video_format = st_video_fmt_descs[i].fmt;
      return ST_JSON_SUCCESS;
    }
  }
  logger->error("{}, invalid video format {}", __func__, video_format);
  return -ST_JSON_NOT_VALID;
}

static int parse_video_pg_format(json_object* video_obj, st_json_video_session_t* video) {
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
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_url(json_object* obj, const char* name, char* url) {
  const char* url_src = json_object_get_string(st_json_object_object_get(obj, name));
  REQUIRED_ITEM(url_src);
  snprintf(url, ST_APP_URL_MAX_LEN, "%s", url_src);
  return -ST_JSON_SUCCESS;
}

static int st_json_parse_tx_video(int idx, json_object* video_obj,
                                  st_json_video_session_t* video) {
  if (video_obj == NULL || video == NULL) {
    logger->error("{}, can not parse tx video session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(video_obj, &video->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(video_obj, &video->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_VIDEO);
    video->base.payload_type = ST_APP_PAYLOAD_TYPE_VIDEO;
  }

  /* parse video type */
  ret = parse_video_type(video_obj, video);
  if (ret < 0) return ret;

  /* parse video pacing */
  ret = parse_video_pacing(video_obj, video);
  if (ret < 0) return ret;

  /* parse video packing mode */
  ret = parse_video_packing(video_obj, video);
  if (ret < 0) return ret;

  /* parse tr offset */
  ret = parse_video_tr_offset(video_obj, video);
  if (ret < 0) return ret;

  /* parse video format */
  ret = parse_video_format(video_obj, video);
  if (ret < 0) return ret;

  /* parse pixel group format */
  ret = parse_video_pg_format(video_obj, video);
  if (ret < 0) return ret;

  /* parse video url */
  ret = parse_url(video_obj, "video_url", video->info.video_url);
  if (ret < 0) return ret;

  return ST_JSON_SUCCESS;
}

static int st_json_parse_rx_video(int idx, json_object* video_obj,
                                  st_json_video_session_t* video) {
  if (video_obj == NULL || video == NULL) {
    logger->error("{}, can not parse rx video session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(video_obj, &video->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(video_obj, &video->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_VIDEO);
    video->base.payload_type = ST_APP_PAYLOAD_TYPE_VIDEO;
  }

  /* parse video type */
  ret = parse_video_type(video_obj, video);
  if (ret < 0) return ret;

  /* parse video pacing */
  ret = parse_video_pacing(video_obj, video);
  if (ret < 0) return ret;

  /* parse tr offset */
  ret = parse_video_tr_offset(video_obj, video);
  if (ret < 0) return ret;

  /* parse video format */
  ret = parse_video_format(video_obj, video);
  if (ret < 0) return ret;

  /* parse pixel group format */
  ret = parse_video_pg_format(video_obj, video);
  if (ret < 0) return ret;

  /* parse user pixel group format */
  video->user_pg_format = USER_FMT_MAX;
  const char* user_pg_format =
      json_object_get_string(st_json_object_object_get(video_obj, "user_pg_format"));
  if (user_pg_format != NULL) {
    if (strcmp(user_pg_format, "YUV_422_8bit") == 0) {
      video->user_pg_format = USER_FMT_YUV_422_8BIT;
    } else {
      logger->error("{}, invalid pixel group format {}", __func__, user_pg_format);
      return -ST_JSON_NOT_VALID;
    }
  }

  /* parse display option */
  video->display =
      json_object_get_boolean(st_json_object_object_get(video_obj, "display"));

  /* parse measure_latency option */
  video->measure_latency =
      json_object_get_boolean(st_json_object_object_get(video_obj, "measure_latency"));

  return ST_JSON_SUCCESS;
}

static int parse_audio_type(json_object* audio_obj, st_json_audio_session_t* audio) {
  const char* type = json_object_get_string(st_json_object_object_get(audio_obj, "type"));
  REQUIRED_ITEM(type);
  if (strcmp(type, "frame") == 0) {
    audio->info.type = ST30_TYPE_FRAME_LEVEL;
  } else if (strcmp(type, "rtp") == 0) {
    audio->info.type = ST30_TYPE_RTP_LEVEL;
  } else {
    logger->error("{}, invalid audio type {}", __func__, type);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_audio_format(json_object* audio_obj, st_json_audio_session_t* audio) {
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
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_audio_channel(json_object* audio_obj, st_json_audio_session_t* audio) {
  json_object* audio_channel_array =
      st_json_object_object_get(audio_obj, "audio_channel");
  if (audio_channel_array == NULL ||
      json_object_get_type(audio_channel_array) != json_type_array) {
    logger->error("{}, can not parse audio channel", __func__);
    return -ST_JSON_PARSE_FAIL;
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
        return -ST_JSON_NOT_VALID;
      }
      audio->info.audio_channel += num_channel;
    } else {
      logger->error("{}, invalid audio channel {}", __func__, channel);
      return -ST_JSON_NOT_VALID;
    }
  }
  return ST_JSON_SUCCESS;
}

static int parse_audio_sampling(json_object* audio_obj, st_json_audio_session_t* audio) {
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
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_audio_ptime(json_object* audio_obj, st_json_audio_session_t* audio) {
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
      return -ST_JSON_NOT_VALID;
    }
  } else { /* default ptime 1ms */
    audio->info.audio_ptime = ST30_PTIME_1MS;
  }
  return ST_JSON_SUCCESS;
}

static int st_json_parse_tx_audio(int idx, json_object* audio_obj,
                                  st_json_audio_session_t* audio) {
  if (audio_obj == NULL || audio == NULL) {
    logger->error("{}, can not parse tx audio session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(audio_obj, &audio->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(audio_obj, &audio->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_AUDIO);
    audio->base.payload_type = ST_APP_PAYLOAD_TYPE_AUDIO;
  }

  /* parse audio type */
  ret = parse_audio_type(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio format */
  ret = parse_audio_format(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio channel */
  ret = parse_audio_channel(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio sampling */
  ret = parse_audio_sampling(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio packet time */
  ret = parse_audio_ptime(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio url */
  ret = parse_url(audio_obj, "audio_url", audio->info.audio_url);
  if (ret < 0) return ret;

  return ST_JSON_SUCCESS;
}

static int st_json_parse_rx_audio(int idx, json_object* audio_obj,
                                  st_json_audio_session_t* audio) {
  if (audio_obj == NULL || audio == NULL) {
    logger->error("{}, can not parse rx audio session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(audio_obj, &audio->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(audio_obj, &audio->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_AUDIO);
    audio->base.payload_type = ST_APP_PAYLOAD_TYPE_AUDIO;
  }

  /* parse audio type */
  ret = parse_audio_type(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio format */
  ret = parse_audio_format(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio channel */
  ret = parse_audio_channel(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio sampling */
  ret = parse_audio_sampling(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio packet time */
  ret = parse_audio_ptime(audio_obj, audio);
  if (ret < 0) return ret;

  /* parse audio url */
  ret = parse_url(audio_obj, "audio_url", audio->info.audio_url);
  if (ret < 0) {
    logger->error("{}, no reference file", __func__);
  }

  return ST_JSON_SUCCESS;
}

static int st_json_parse_tx_anc(int idx, json_object* anc_obj,
                                st_json_ancillary_session_t* anc) {
  if (anc_obj == NULL || anc == NULL) {
    logger->error("{}, can not parse tx anc session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(anc_obj, &anc->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(anc_obj, &anc->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_ANCILLARY);
    anc->base.payload_type = ST_APP_PAYLOAD_TYPE_ANCILLARY;
  }

  /* parse anc type */
  const char* type = json_object_get_string(st_json_object_object_get(anc_obj, "type"));
  REQUIRED_ITEM(type);
  if (strcmp(type, "frame") == 0) {
    anc->info.type = ST40_TYPE_FRAME_LEVEL;
  } else if (strcmp(type, "rtp") == 0) {
    anc->info.type = ST40_TYPE_RTP_LEVEL;
  } else {
    logger->error("{}, invalid anc type {}", __func__, type);
    return -ST_JSON_NOT_VALID;
  }
  /* parse anc format */
  const char* anc_format =
      json_object_get_string(st_json_object_object_get(anc_obj, "ancillary_format"));
  REQUIRED_ITEM(anc_format);
  if (strcmp(anc_format, "closed_caption") == 0) {
    anc->info.anc_format = ANC_FORMAT_CLOSED_CAPTION;
  } else {
    logger->error("{}, invalid anc format {}", __func__, anc_format);
    return -ST_JSON_NOT_VALID;
  }

  /* parse anc fps */
  const char* anc_fps =
      json_object_get_string(st_json_object_object_get(anc_obj, "ancillary_fps"));
  REQUIRED_ITEM(anc_fps);
  if (strcmp(anc_fps, "p59") == 0) {
    anc->info.anc_fps = ST_FPS_P59_94;
  } else if (strcmp(anc_fps, "p50") == 0) {
    anc->info.anc_fps = ST_FPS_P50;
  } else if (strcmp(anc_fps, "p25") == 0) {
    anc->info.anc_fps = ST_FPS_P25;
  } else if (strcmp(anc_fps, "p29") == 0) {
    anc->info.anc_fps = ST_FPS_P29_97;
  } else {
    logger->error("{}, invalid anc fps {}", __func__, anc_fps);
    return -ST_JSON_NOT_VALID;
  }

  /* parse anc url */
  ret = parse_url(anc_obj, "ancillary_url", anc->info.anc_url);
  if (ret < 0) return ret;

  return ST_JSON_SUCCESS;
}

static int st_json_parse_rx_anc(int idx, json_object* anc_obj,
                                st_json_ancillary_session_t* anc) {
  if (anc_obj == NULL || anc == NULL) {
    logger->error("{}, can not parse rx anc session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(anc_obj, &anc->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(anc_obj, &anc->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_ANCILLARY);
    anc->base.payload_type = ST_APP_PAYLOAD_TYPE_ANCILLARY;
  }

  return ST_JSON_SUCCESS;
}

static int parse_st22p_width(json_object* st22p_obj, st_json_st22p_session_t* st22p) {
  int width = json_object_get_int(st_json_object_object_get(st22p_obj, "width"));
  if (width <= 0) {
    logger->error("{}, invalid width {}", __func__, width);
    return -ST_JSON_NOT_VALID;
  }
  st22p->info.width = width;
  return ST_JSON_SUCCESS;
}

static int parse_st22p_height(json_object* st22p_obj, st_json_st22p_session_t* st22p) {
  int height = json_object_get_int(st_json_object_object_get(st22p_obj, "height"));
  if (height <= 0) {
    logger->error("{}, invalid height {}", __func__, height);
    return -ST_JSON_NOT_VALID;
  }
  st22p->info.height = height;
  return ST_JSON_SUCCESS;
}

static int parse_st22p_fps(json_object* st22p_obj, st_json_st22p_session_t* st22p) {
  const char* fps = json_object_get_string(st_json_object_object_get(st22p_obj, "fps"));
  REQUIRED_ITEM(fps);
  if (strcmp(fps, "p59") == 0) {
    st22p->info.fps = ST_FPS_P59_94;
  } else if (strcmp(fps, "p50") == 0) {
    st22p->info.fps = ST_FPS_P50;
  } else if (strcmp(fps, "p25") == 0) {
    st22p->info.fps = ST_FPS_P25;
  } else if (strcmp(fps, "p29") == 0) {
    st22p->info.fps = ST_FPS_P29_97;
  } else {
    logger->error("{}, invalid anc fps {}", __func__, fps);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_st22p_pack_type(json_object* st22p_obj, st_json_st22p_session_t* st22p) {
  const char* pack_type =
      json_object_get_string(st_json_object_object_get(st22p_obj, "pack_type"));
  REQUIRED_ITEM(pack_type);
  if (strcmp(pack_type, "codestream") == 0) {
    st22p->info.pack_type = ST22_PACK_CODESTREAM;
  } else if (strcmp(pack_type, "slice") == 0) {
    st22p->info.pack_type = ST22_PACK_SLICE;
  } else {
    logger->error("{}, invalid pack_type {}", __func__, pack_type);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_st22p_codec(json_object* st22p_obj, st_json_st22p_session_t* st22p) {
  const char* codec =
      json_object_get_string(st_json_object_object_get(st22p_obj, "codec"));
  REQUIRED_ITEM(codec);
  if (strcmp(codec, "JPEG-XS") == 0) {
    st22p->info.codec = ST22_CODEC_JPEGXS;
  } else {
    logger->error("{}, invalid codec {}", __func__, codec);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_st22p_device(json_object* st22p_obj, st_json_st22p_session_t* st22p) {
  const char* device =
      json_object_get_string(st_json_object_object_get(st22p_obj, "device"));
  REQUIRED_ITEM(device);
  if (strcmp(device, "AUTO") == 0) {
    st22p->info.device = ST_PLUGIN_DEVICE_AUTO;
  } else if (strcmp(device, "CPU") == 0) {
    st22p->info.device = ST_PLUGIN_DEVICE_CPU;
  } else if (strcmp(device, "GPU") == 0) {
    st22p->info.device = ST_PLUGIN_DEVICE_GPU;
  } else if (strcmp(device, "FPGA") == 0) {
    st22p->info.device = ST_PLUGIN_DEVICE_FPGA;
  } else {
    logger->error("{}, invalid plugin device type {}", __func__, device);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_st22p_quality(json_object* st22p_obj, st_json_st22p_session_t* st22p) {
  const char* quality =
      json_object_get_string(st_json_object_object_get(st22p_obj, "quality"));
  if (quality) {
    if (strcmp(quality, "quality") == 0) {
      st22p->info.quality = ST22_QUALITY_MODE_QUALITY;
    } else if (strcmp(quality, "speed") == 0) {
      st22p->info.quality = ST22_QUALITY_MODE_SPEED;
    } else {
      logger->error("{}, invalid plugin quality type {}", __func__, quality);
      return -ST_JSON_NOT_VALID;
    }
  } else { /* default use speed mode */
    st22p->info.quality = ST22_QUALITY_MODE_SPEED;
  }
  return ST_JSON_SUCCESS;
}

static int parse_st22p_format(json_object* st22p_obj, st_json_st22p_session_t* st22p,
                              const char* format_name) {
  const char* format =
      json_object_get_string(st_json_object_object_get(st22p_obj, format_name));
  REQUIRED_ITEM(format);
  if (strcmp(format, "YUV422PLANAR10LE") == 0) {
    st22p->info.format = ST_FRAME_FMT_YUV422PLANAR10LE;
  } else if (strcmp(format, "ARGB") == 0) {
    st22p->info.format = ST_FRAME_FMT_ARGB;
  } else if (strcmp(format, "BGRA") == 0) {
    st22p->info.format = ST_FRAME_FMT_BGRA;
  } else if (strcmp(format, "V210") == 0) {
    st22p->info.format = ST_FRAME_FMT_V210;
  } else if (strcmp(format, "YUV422PLANAR8") == 0) {
    st22p->info.format = ST_FRAME_FMT_YUV422PLANAR8;
  } else if (strcmp(format, "YUV422PACKED8") == 0) {
    st22p->info.format = ST_FRAME_FMT_UYVY;
  } else if (strcmp(format, "YUV422RFC4175PG2BE10") == 0) {
    st22p->info.format = ST_FRAME_FMT_YUV422RFC4175PG2BE10;
  } else if (strcmp(format, "RGB8") == 0) {
    st22p->info.format = ST_FRAME_FMT_RGB8;
  } else if (strcmp(format, "JPEGXS_CODESTREAM") == 0) {
    st22p->info.format = ST_FRAME_FMT_JPEGXS_CODESTREAM;
  } else {
    logger->error("{}, invalid output format {}", __func__, format);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int st_json_parse_tx_st22p(int idx, json_object* st22p_obj,
                                  st_json_st22p_session_t* st22p) {
  if (st22p_obj == NULL || st22p == NULL) {
    logger->error("{}, can not parse tx st22p session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(st22p_obj, &st22p->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(st22p_obj, &st22p->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_ST22);
    st22p->base.payload_type = ST_APP_PAYLOAD_TYPE_ST22;
  }

  /* parse width */
  ret = parse_st22p_width(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse height */
  ret = parse_st22p_height(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse fps */
  ret = parse_st22p_fps(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse pack_type */
  ret = parse_st22p_pack_type(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse codec */
  ret = parse_st22p_codec(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse device */
  ret = parse_st22p_device(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse quality */
  ret = parse_st22p_quality(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse input format */
  ret = parse_st22p_format(st22p_obj, st22p, "input_format");
  if (ret < 0) return ret;

  /* parse st22p url */
  ret = parse_url(st22p_obj, "st22p_url", st22p->info.st22p_url);
  if (ret < 0) return ret;

  /* parse codec_thread_count option */
  st22p->info.codec_thread_count =
      json_object_get_int(st_json_object_object_get(st22p_obj, "codec_thread_count"));

  return ST_JSON_SUCCESS;
}

static int st_json_parse_rx_st22p(int idx, json_object* st22p_obj,
                                  st_json_st22p_session_t* st22p) {
  if (st22p_obj == NULL || st22p == NULL) {
    logger->error("{}, can not parse rx st22p session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(st22p_obj, &st22p->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(st22p_obj, &st22p->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_ST22);
    st22p->base.payload_type = ST_APP_PAYLOAD_TYPE_ST22;
  }

  /* parse width */
  ret = parse_st22p_width(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse height */
  ret = parse_st22p_height(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse fps */
  ret = parse_st22p_fps(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse pack_type */
  ret = parse_st22p_pack_type(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse codec */
  ret = parse_st22p_codec(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse device */
  ret = parse_st22p_device(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse quality */
  ret = parse_st22p_quality(st22p_obj, st22p);
  if (ret < 0) return ret;

  /* parse output format */
  ret = parse_st22p_format(st22p_obj, st22p, "output_format");
  if (ret < 0) return ret;

  /* parse display option */
  st22p->display =
      json_object_get_boolean(st_json_object_object_get(st22p_obj, "display"));

  /* parse measure_latency option */
  st22p->measure_latency =
      json_object_get_boolean(st_json_object_object_get(st22p_obj, "measure_latency"));

  /* parse codec_thread_count option */
  st22p->info.codec_thread_count =
      json_object_get_int(st_json_object_object_get(st22p_obj, "codec_thread_count"));

  return ST_JSON_SUCCESS;
}

static int parse_st20p_width(json_object* st20p_obj, st_json_st20p_session_t* st20p) {
  int width = json_object_get_int(st_json_object_object_get(st20p_obj, "width"));
  if (width <= 0) {
    logger->error("{}, invalid width {}", __func__, width);
    return -ST_JSON_NOT_VALID;
  }
  st20p->info.width = width;
  return ST_JSON_SUCCESS;
}

static int parse_st20p_height(json_object* st20p_obj, st_json_st20p_session_t* st20p) {
  int height = json_object_get_int(st_json_object_object_get(st20p_obj, "height"));
  if (height <= 0) {
    logger->error("{}, invalid height {}", __func__, height);
    return -ST_JSON_NOT_VALID;
  }
  st20p->info.height = height;
  return ST_JSON_SUCCESS;
}

static int parse_st20p_fps(json_object* st20p_obj, st_json_st20p_session_t* st20p) {
  const char* fps = json_object_get_string(st_json_object_object_get(st20p_obj, "fps"));
  REQUIRED_ITEM(fps);
  if (strcmp(fps, "p59") == 0) {
    st20p->info.fps = ST_FPS_P59_94;
  } else if (strcmp(fps, "p50") == 0) {
    st20p->info.fps = ST_FPS_P50;
  } else if (strcmp(fps, "p25") == 0) {
    st20p->info.fps = ST_FPS_P25;
  } else if (strcmp(fps, "p29") == 0) {
    st20p->info.fps = ST_FPS_P29_97;
  } else {
    logger->error("{}, invalid anc fps {}", __func__, fps);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_st20p_device(json_object* st20p_obj, st_json_st20p_session_t* st20p) {
  const char* device =
      json_object_get_string(st_json_object_object_get(st20p_obj, "device"));
  REQUIRED_ITEM(device);
  if (strcmp(device, "AUTO") == 0) {
    st20p->info.device = ST_PLUGIN_DEVICE_AUTO;
  } else if (strcmp(device, "CPU") == 0) {
    st20p->info.device = ST_PLUGIN_DEVICE_CPU;
  } else if (strcmp(device, "GPU") == 0) {
    st20p->info.device = ST_PLUGIN_DEVICE_GPU;
  } else if (strcmp(device, "FPGA") == 0) {
    st20p->info.device = ST_PLUGIN_DEVICE_FPGA;
  } else {
    logger->error("{}, invalid plugin device type {}", __func__, device);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_st20p_format(json_object* st20p_obj, st_json_st20p_session_t* st20p,
                              const char* format_name) {
  const char* format =
      json_object_get_string(st_json_object_object_get(st20p_obj, format_name));
  REQUIRED_ITEM(format);
  if (strcmp(format, "YUV422PLANAR10LE") == 0) {
    st20p->info.format = ST_FRAME_FMT_YUV422PLANAR10LE;
  } else if (strcmp(format, "ARGB") == 0) {
    st20p->info.format = ST_FRAME_FMT_ARGB;
  } else if (strcmp(format, "BGRA") == 0) {
    st20p->info.format = ST_FRAME_FMT_BGRA;
  } else if (strcmp(format, "V210") == 0) {
    st20p->info.format = ST_FRAME_FMT_V210;
  } else if (strcmp(format, "YUV422PLANAR8") == 0) {
    st20p->info.format = ST_FRAME_FMT_YUV422PLANAR8;
  } else if (strcmp(format, "YUV422PACKED8") == 0) {
    st20p->info.format = ST_FRAME_FMT_UYVY;
  } else if (strcmp(format, "YUV422RFC4175PG2BE10") == 0) {
    st20p->info.format = ST_FRAME_FMT_YUV422RFC4175PG2BE10;
  } else if (strcmp(format, "RGB8") == 0) {
    st20p->info.format = ST_FRAME_FMT_RGB8;
  } else {
    logger->error("{}, invalid output format {}", __func__, format);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int parse_st20p_transport_format(json_object* st20p_obj,
                                        st_json_st20p_session_t* st20p) {
  const char* t_format =
      json_object_get_string(st_json_object_object_get(st20p_obj, "transport_format"));
  REQUIRED_ITEM(t_format);
  if (strcmp(t_format, "YUV_422_10bit") == 0) {
    st20p->info.transport_format = ST20_FMT_YUV_422_10BIT;
  } else if (strcmp(t_format, "YUV_422_8bit") == 0) {
    st20p->info.transport_format = ST20_FMT_YUV_422_8BIT;
  } else if (strcmp(t_format, "YUV_422_12bit") == 0) {
    st20p->info.transport_format = ST20_FMT_YUV_422_12BIT;
  } else if (strcmp(t_format, "YUV_422_16bit") == 0) {
    st20p->info.transport_format = ST20_FMT_YUV_422_16BIT;
  } else if (strcmp(t_format, "YUV_420_8bit") == 0) {
    st20p->info.transport_format = ST20_FMT_YUV_420_8BIT;
  } else if (strcmp(t_format, "YUV_420_10bit") == 0) {
    st20p->info.transport_format = ST20_FMT_YUV_420_10BIT;
  } else if (strcmp(t_format, "YUV_420_12bit") == 0) {
    st20p->info.transport_format = ST20_FMT_YUV_420_12BIT;
  } else if (strcmp(t_format, "RGB_8bit") == 0) {
    st20p->info.transport_format = ST20_FMT_RGB_8BIT;
  } else if (strcmp(t_format, "RGB_10bit") == 0) {
    st20p->info.transport_format = ST20_FMT_RGB_10BIT;
  } else if (strcmp(t_format, "RGB_12bit") == 0) {
    st20p->info.transport_format = ST20_FMT_RGB_12BIT;
  } else if (strcmp(t_format, "RGB_16bit") == 0) {
    st20p->info.transport_format = ST20_FMT_RGB_16BIT;
  } else {
    logger->error("{}, invalid transport format {}", __func__, t_format);
    return -ST_JSON_NOT_VALID;
  }
  return ST_JSON_SUCCESS;
}

static int st_json_parse_tx_st20p(int idx, json_object* st20p_obj,
                                  st_json_st20p_session_t* st20p) {
  if (st20p_obj == NULL || st20p == NULL) {
    logger->error("{}, can not parse tx st20p session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(st20p_obj, &st20p->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(st20p_obj, &st20p->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_ST22);
    st20p->base.payload_type = ST_APP_PAYLOAD_TYPE_ST22;
  }

  /* parse width */
  ret = parse_st20p_width(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse height */
  ret = parse_st20p_height(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse fps */
  ret = parse_st20p_fps(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse device */
  ret = parse_st20p_device(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse input format */
  ret = parse_st20p_format(st20p_obj, st20p, "input_format");
  if (ret < 0) return ret;

  /* parse transport format */
  ret = parse_st20p_transport_format(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse st20p url */
  ret = parse_url(st20p_obj, "st20p_url", st20p->info.st20p_url);
  if (ret < 0) return ret;

  return ST_JSON_SUCCESS;
}

static int st_json_parse_rx_st20p(int idx, json_object* st20p_obj,
                                  st_json_st20p_session_t* st20p) {
  if (st20p_obj == NULL || st20p == NULL) {
    logger->error("{}, can not parse rx st20p session", __func__);
    return -ST_JSON_NULL;
  }
  int ret;

  /* parse udp port  */
  ret = parse_base_udp_port(st20p_obj, &st20p->base, idx);
  if (ret < 0) return ret;

  /* parse payload type */
  ret = parse_base_payload_type(st20p_obj, &st20p->base);
  if (ret < 0) {
    logger->error("{}, use default pt {}", __func__, ST_APP_PAYLOAD_TYPE_ST22);
    st20p->base.payload_type = ST_APP_PAYLOAD_TYPE_ST22;
  }

  /* parse width */
  ret = parse_st20p_width(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse height */
  ret = parse_st20p_height(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse fps */
  ret = parse_st20p_fps(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse device */
  ret = parse_st20p_device(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse output format */
  ret = parse_st20p_format(st20p_obj, st20p, "output_format");
  if (ret < 0) return ret;

  /* parse transport format */
  ret = parse_st20p_transport_format(st20p_obj, st20p);
  if (ret < 0) return ret;

  /* parse display option */
  st20p->display =
      json_object_get_boolean(st_json_object_object_get(st20p_obj, "display"));

  /* parse measure_latency option */
  st20p->measure_latency =
      json_object_get_boolean(st_json_object_object_get(st20p_obj, "measure_latency"));

  return ST_JSON_SUCCESS;
}

static int parse_session_num(json_object* group, const char* name) {
  int num = 0;
  json_object* session_array = st_json_object_object_get(group, name);
  if (session_array != NULL && json_object_get_type(session_array) == json_type_array) {
    for (int j = 0; j < json_object_array_length(session_array); ++j) {
      json_object* session = json_object_array_get_idx(session_array, j);
      int replicas = json_object_get_int(st_json_object_object_get(session, "replicas"));
      if (replicas < 0) {
        logger->error("{}, invalid replicas number: {}", __func__, replicas);
        return -ST_JSON_NOT_VALID;
      }
      num += replicas;
    }
  }
  return num;
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

void json_put_string(json_object *o, const std::string &key, const std::string &value) {
  json_object *val = json_object_new_string(value.c_str());
  if (json_object_object_add(o, key.c_str(), val)) {
    json_object_put(val);
  }
}

int st_app_parse_json_sch(st_json_context_t* ctx, json_object *root_object) {
  /* parse quota for system */
  json_object* sch_quota_object =
      st_json_object_object_get(root_object, "sch_session_quota");
  if (sch_quota_object != NULL) {
    int sch_quota = json_object_get_int(sch_quota_object);
    if (sch_quota <= 0) {
      logger->error("{}, invalid quota number", __func__);
      return -ST_JSON_NOT_VALID;
    }
    ctx->sch_quota = sch_quota;
  }
  return ST_JSON_SUCCESS;
}

int st_app_parse_json_interfaces(st_json_context_t* ctx, json_object *root_object) {
  int ret = ST_JSON_SUCCESS;
  /* parse interfaces for system */
  json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
  if (interfaces_array == NULL ||
      json_object_get_type(interfaces_array) != json_type_array) {
    logger->error("{}, can not parse interfaces", __func__);
    return -ST_JSON_PARSE_FAIL;
  }
  int num_interfaces = json_object_array_length(interfaces_array);
  ctx->interfaces.resize(num_interfaces);
  for (int i = 0; i < num_interfaces; ++i) {
    ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
                                   &ctx->interfaces[i]);
    if (ret) return ret;
  }
  return ret;
}

int st_app_parse_json_tx_sessions(st_json_context_t* ctx, json_object *root_object) {
  int ret = ST_JSON_SUCCESS;
  int num_interfaces = ctx->interfaces.size();
  /* parse tx sessions  */
  json_object* tx_group_array = st_json_object_object_get(root_object, "tx_sessions");
  if (tx_group_array != NULL && json_object_get_type(tx_group_array) == json_type_array) {
    /* allocate tx source config*/
    int tx_source_cnt = json_object_array_length(tx_group_array);
    ctx->tx_sources.resize(tx_source_cnt);
    
    // auto sz = sizeof(struct st_app_tx_source);
    // auto sc = ( st_app_tx_source*)st_app_zmalloc(ctx->tx_source_cnt*sizeof( st_app_tx_source));
    // auto str_sz = sizeof(std::string);
    // sc[0].device_id = 1;
    // sc[0].type = "type";
    // sc[0].file_url = "file_url";
    // sc[0].video_format = "video_format";
    // sc[1].device_id = 2;
    // sc[1].type = "type1";
    // sc[1].file_url = "file_url1";
    // sc[1].video_format = "video_format1";
    // logger->info("{}, {},{},{}", __func__, sc[0].device_id, sc[0].type, sc[0].video_format);
    // logger->info("{}, {},{},{}", __func__, sc[1].device_id, sc[1].type, sc[1].video_format);
    
    int tx_video_cnt = 0;
    int tx_audio_cnt = 0;
    int tx_ancillary_cnt = 0;
    int tx_st22p_cnt = 0;
    int tx_st20p_cnt = 0;
    /* parse session numbers for array allocation */
    for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
      json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
      if (tx_group == NULL) {
        logger->error("{}, can not parse tx session group", __func__);
        return -ST_JSON_PARSE_FAIL;
      }
      int num = 0;
      /* parse tx video sessions */
      num = parse_session_num(tx_group, "video");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      tx_video_cnt += (num);
      /* parse tx audio sessions */
      num = parse_session_num(tx_group, "audio");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      tx_audio_cnt += (num);
      /* parse tx ancillary sessions */
      num = parse_session_num(tx_group, "ancillary");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      tx_ancillary_cnt += (num);
      /* parse tx st22p sessions */
      num = parse_session_num(tx_group, "st22p");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      tx_st22p_cnt += (num);
      /* parse tx st20p sessions */
      num = parse_session_num(tx_group, "st20p");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      tx_st20p_cnt += (num);

      // parse tx source config
      json_object* source = st_json_object_object_get(tx_group, "source");
      auto &tx_source = ctx->tx_sources[i];
      tx_source.type = safe_get_string(source, "type");
      tx_source.device_id = json_object_get_int(st_json_object_object_get(source, "device_id"));
      tx_source.file_url = safe_get_string(source, "file_url");
      tx_source.video_format = safe_get_string(source, "video_format");
      tx_source.id = safe_get_string(source, "id");
      if (tx_source.id.empty()) {
        tx_source.id = seeder::core::generate_uuid();
      }
    }

    std::unordered_map<std::string, int> source_id_map;
    for (auto &tx_source : ctx->tx_sources) {
      if (tx_source.id.empty()) {
        logger->error("empty tx_source.id {}", tx_source.id);
        return -ST_JSON_NOT_VALID;
      }
      if (++source_id_map[tx_source.id] > 1) {
        logger->error("duplicated tx_source.id {}", tx_source.id);
        return -ST_JSON_NOT_VALID;
      }
    }

    ctx->tx_video_sessions.resize(tx_video_cnt);
    ctx->tx_audio_sessions.resize(tx_audio_cnt);
    ctx->tx_anc_sessions.resize(tx_ancillary_cnt);
    ctx->tx_st22p_sessions.resize(tx_st22p_cnt);
    ctx->tx_st20p_sessions.resize(tx_st20p_cnt);
    
    int num_inf = 0;
    int num_video = 0;
    int num_audio = 0;
    int num_anc = 0;
    int num_st22p = 0;
    int num_st20p = 0;

    for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
      json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
      if (tx_group == NULL) {
        logger->error("{}, can not parse tx session group", __func__);
        return -ST_JSON_PARSE_FAIL;
      }

      auto &tx_source = ctx->tx_sources[i];
      json_put_string(tx_group, "id", tx_source.id);

      /* parse destination ip */
      json_object* dip_p = NULL;
      json_object* dip_r = NULL;
      json_object* dip_array = st_json_object_object_get(tx_group, "dip");
      if (dip_array != NULL && json_object_get_type(dip_array) == json_type_array) {
        int len = json_object_array_length(dip_array);
        if (len < 1 || len > MTL_PORT_MAX) {
          logger->error("{}, wrong dip number", __func__);
          return -ST_JSON_NOT_VALID;
        }
        dip_p = json_object_array_get_idx(dip_array, 0);
        if (len == 2) {
          dip_r = json_object_array_get_idx(dip_array, 1);
        }
        num_inf = len;
      } else {
        logger->error("{}, can not parse dip_array", __func__);
        return -ST_JSON_PARSE_FAIL;
      }

      /* parse interface */
      int inf_p, inf_r = 0;
      json_object* interface_array = st_json_object_object_get(tx_group, "interface");
      if (interface_array != NULL &&
          json_object_get_type(interface_array) == json_type_array) {
        int len = json_object_array_length(interface_array);
        if (len != num_inf) {
          logger->error("{}, wrong interface number", __func__);
          return -ST_JSON_NOT_VALID;
        }
        inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
        if (inf_p < 0 || inf_p > num_interfaces) {
          logger->error("{}, wrong interface index", __func__);
          return -ST_JSON_NOT_VALID;
        }
        if (len == 2) {
          inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
          if (inf_r < 0 || inf_r > num_interfaces) {
            logger->error("{}, wrong interface index", __func__);
            return -ST_JSON_NOT_VALID;
          }
        }
      } else {
        logger->error("{}, can not parse interface_array", __func__);
        return -ST_JSON_PARSE_FAIL;
      }

      /* parse tx video sessions */
      json_object* video_array = st_json_object_object_get(tx_group, "video");
      if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(video_array); ++j) {
          json_object* video_session = json_object_array_get_idx(video_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(video_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          // 取video对象的dip，没有的话用外层的dip
          json_object* video_dip_p = NULL;
          json_object* video_dip_r = NULL;
          json_object* video_dip_array = st_json_object_object_get(video_session, "dip");
          if (video_dip_array != NULL && json_object_get_type(video_dip_array) == json_type_array) {
            int len = json_object_array_length(video_dip_array);
            if (len < 1 || len > MTL_PORT_MAX) {
              logger->error("{}, wrong dip number", __func__);
              return -ST_JSON_NOT_VALID;
            }
            video_dip_p = json_object_array_get_idx(video_dip_array, 0);
            if (len == 2) {
              video_dip_r = json_object_array_get_idx(video_dip_array, 1);
            }
          } else {
            video_dip_p = dip_p;
            video_dip_r = dip_r;
          }

          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(video_dip_p),
                      ctx->tx_video_sessions[num_video].base.ip[0]);
            ctx->tx_video_sessions[num_video].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(video_dip_r),
                        ctx->tx_video_sessions[num_video].base.ip[1]);
              ctx->tx_video_sessions[num_video].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->tx_video_sessions[num_video].base.num_inf = num_inf;
            ret = st_json_parse_tx_video(k, video_session,
                                         &ctx->tx_video_sessions[num_video]);
            if (ret) return ret;

            // video source handle id
            ctx->tx_video_sessions[num_video].tx_source_id = tx_source.id;

            num_video++;
          }
        }
      }

      /* parse tx audio sessions */
      json_object* audio_array = st_json_object_object_get(tx_group, "audio");
      if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(audio_array); ++j) {
          json_object* audio_session = json_object_array_get_idx(audio_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          // 取audio对象的dip，没有的话用外层的dip
          json_object* audio_dip_p = NULL;
          json_object* audio_dip_r = NULL;
          json_object* audio_dip_array = st_json_object_object_get(audio_session, "dip");
          if (audio_dip_array != NULL && json_object_get_type(audio_dip_array) == json_type_array) {
            int len = json_object_array_length(audio_dip_array);
            if (len < 1 || len > MTL_PORT_MAX) {
              logger->error("{}, wrong dip number", __func__);
              return -ST_JSON_NOT_VALID;
            }
            audio_dip_p = json_object_array_get_idx(audio_dip_array, 0);
            if (len == 2) {
              audio_dip_r = json_object_array_get_idx(audio_dip_array, 1);
            }
          } else {
            audio_dip_p = dip_p;
            audio_dip_r = dip_r;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(audio_dip_p),
                      ctx->tx_audio_sessions[num_audio].base.ip[0]);
            ctx->tx_audio_sessions[num_audio].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(audio_dip_r),
                        ctx->tx_audio_sessions[num_audio].base.ip[1]);
              ctx->tx_audio_sessions[num_audio].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->tx_audio_sessions[num_audio].base.num_inf = num_inf;
            ret = st_json_parse_tx_audio(k, audio_session,
                                         &ctx->tx_audio_sessions[num_audio]);
            if (ret) return ret;

            // audio source handle id
            ctx->tx_audio_sessions[num_audio].tx_source_id = tx_source.id;

            num_audio++;
          }
        }
      }

      /* parse tx ancillary sessions */
      json_object* anc_array = st_json_object_object_get(tx_group, "ancillary");
      if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(anc_array); ++j) {
          json_object* anc_session = json_object_array_get_idx(anc_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(dip_p),
                      ctx->tx_anc_sessions[num_anc].base.ip[0]);
            ctx->tx_anc_sessions[num_anc].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(dip_r),
                        ctx->tx_anc_sessions[num_anc].base.ip[1]);
              ctx->tx_anc_sessions[num_anc].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->tx_anc_sessions[num_anc].base.num_inf = num_inf;
            ret = st_json_parse_tx_anc(k, anc_session, &ctx->tx_anc_sessions[num_anc]);
            if (ret) return ret;
            num_anc++;
          }
        }
      }

      /* parse tx st22p sessions */
      json_object* st22p_array = st_json_object_object_get(tx_group, "st22p");
      if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
          json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(dip_p),
                      ctx->tx_st22p_sessions[num_st22p].base.ip[0]);
            ctx->tx_st22p_sessions[num_st22p].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(dip_r),
                        ctx->tx_st22p_sessions[num_st22p].base.ip[1]);
              ctx->tx_st22p_sessions[num_st22p].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->tx_st22p_sessions[num_st22p].base.num_inf = num_inf;
            ret = st_json_parse_tx_st22p(k, st22p_session,
                                         &ctx->tx_st22p_sessions[num_st22p]);
            if (ret) return ret;
            num_st22p++;
          }
        }
      }

      /* parse tx st20p sessions */
      json_object* st20p_array = st_json_object_object_get(tx_group, "st20p");
      if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
          json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(dip_p),
                      ctx->tx_st20p_sessions[num_st20p].base.ip[0]);
            ctx->tx_st20p_sessions[num_st20p].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(dip_r),
                        ctx->tx_st20p_sessions[num_st20p].base.ip[1]);
              ctx->tx_st20p_sessions[num_st20p].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->tx_st20p_sessions[num_st20p].base.num_inf = num_inf;
            ret = st_json_parse_tx_st20p(k, st20p_session,
                                         &ctx->tx_st20p_sessions[num_st20p]);
            if (ret) return ret;
            num_st20p++;
          }
        }
      }
    }
  }
  return ret;
}

int st_app_parse_json_rx_sessions(st_json_context_t* ctx, json_object *root_object) {
  int ret = ST_JSON_SUCCESS;
  int num_interfaces = ctx->interfaces.size();

  int rx_video_session_cnt = 0;
  int rx_audio_session_cnt = 0;
  int rx_anc_session_cnt = 0;
  int rx_st22p_session_cnt = 0;
  int rx_st20p_session_cnt = 0;
  /* parse rx sessions */
  json_object* rx_group_array = st_json_object_object_get(root_object, "rx_sessions");
  if (rx_group_array != NULL && json_object_get_type(rx_group_array) == json_type_array) {
    /* allocate rx output config*/
    int rx_output_cnt = json_object_array_length(rx_group_array);
    ctx->rx_output.resize(rx_output_cnt);

    /* parse session numbers for array allocation */
    for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
      json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
      if (rx_group == NULL) {
        logger->error("{}, can not parse rx session group", __func__);
        return -ST_JSON_PARSE_FAIL;
      }
      int num = 0;
      /* parse rx video sessions */
      num = parse_session_num(rx_group, "video");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      rx_video_session_cnt += num;
      /* parse rx audio sessions */
      num = parse_session_num(rx_group, "audio");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      rx_audio_session_cnt += num;
      /* parse rx ancillary sessions */
      num = parse_session_num(rx_group, "ancillary");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      rx_anc_session_cnt += num;
      /* parse rx st22p sessions */
      num = parse_session_num(rx_group, "st22p");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      rx_st22p_session_cnt += num;
      /* parse rx st20p sessions */
      num = parse_session_num(rx_group, "st20p");
      if (num < 0) return -ST_JSON_PARSE_FAIL;
      rx_st20p_session_cnt += num;

      // parse rx output config
      json_object* output = st_json_object_object_get(rx_group, "output");
      auto &rx_output = ctx->rx_output[i];
      rx_output.type = safe_get_string(output, "type");
      rx_output.device_id = json_object_get_int(st_json_object_object_get(output, "device_id"));
      std::string fu = safe_get_string(output, "file_url");
      if(!(fu.empty())) rx_output.file_url = fu;
      rx_output.video_format = safe_get_string(output, "video_format");
      if (rx_output.id.empty()) {
        rx_output.id = seeder::core::generate_uuid();
      }
      rx_output.pixel_format = safe_get_string(output, "pixel_format");
    }

    std::unordered_map<std::string, int> output_id_map;
    for (auto &rx_output : ctx->rx_output) {
      if (rx_output.id.empty()) {
        logger->error("empty rx_output.id");
        return -ST_JSON_NOT_VALID;
      }
      if (++output_id_map[rx_output.id] > 1) {
        logger->error("duplicated rx_output.id {}", rx_output.id);
        return -ST_JSON_NOT_VALID;
      }
    }

    /* allocate tx sessions */
    ctx->rx_video_sessions.resize(rx_video_session_cnt);
    ctx->rx_audio_sessions.resize(rx_audio_session_cnt);
    ctx->rx_anc_sessions.resize(rx_anc_session_cnt);
    ctx->rx_st22p_sessions.resize(rx_st22p_session_cnt);
    ctx->rx_st20p_sessions.resize(rx_st20p_session_cnt);

    int num_inf = 0;
    int num_video = 0;
    int num_audio = 0;
    int num_anc = 0;
    int num_st22p = 0;
    int num_st20p = 0;

    for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
      json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
      if (rx_group == NULL) {
        logger->error("{}, can not parse rx session group", __func__);
        return -ST_JSON_PARSE_FAIL;
      }

      auto &rx_output = ctx->rx_output[i];

      /* parse receiving ip */
      json_object* ip_p = NULL;
      json_object* ip_r = NULL;
      json_object* ip_array = st_json_object_object_get(rx_group, "ip");
      if (ip_array != NULL && json_object_get_type(ip_array) == json_type_array) {
        int len = json_object_array_length(ip_array);
        if (len < 1 || len > MTL_PORT_MAX) {
          logger->error("{}, wrong dip number", __func__);
          return -ST_JSON_NOT_VALID;
        }
        ip_p = json_object_array_get_idx(ip_array, 0);
        if (len == 2) {
          ip_r = json_object_array_get_idx(ip_array, 1);
        }
        num_inf = len;
      } else {
        logger->error("{}, can not parse dip_array", __func__);
        return -ST_JSON_PARSE_FAIL;
      }

      /* parse interface */
      int inf_p, inf_r = 0;
      json_object* interface_array = st_json_object_object_get(rx_group, "interface");
      if (interface_array != NULL &&
          json_object_get_type(interface_array) == json_type_array) {
        int len = json_object_array_length(interface_array);
        if (len != num_inf) {
          logger->error("{}, wrong interface number", __func__);
          return -ST_JSON_NOT_VALID;
        }
        inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
        if (inf_p < 0 || inf_p > num_interfaces) {
          logger->error("{}, wrong interface index", __func__);
          return -ST_JSON_NOT_VALID;
        }
        if (len == 2) {
          inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
          if (inf_r < 0 || inf_r > num_interfaces) {
            logger->error("{}, wrong interface index", __func__);
            return -ST_JSON_NOT_VALID;
          }
        }
      } else {
        logger->error("{}, can not parse interface_array", __func__);
        return -ST_JSON_PARSE_FAIL;
      }

      /* parse rx video sessions */
      json_object* video_array = st_json_object_object_get(rx_group, "video");
      if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(video_array); ++j) {
          json_object* video_session = json_object_array_get_idx(video_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(video_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(ip_p),
                      ctx->rx_video_sessions[num_video].base.ip[0]);
            ctx->rx_video_sessions[num_video].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(ip_r),
                        ctx->rx_video_sessions[num_video].base.ip[1]);
              ctx->rx_video_sessions[num_video].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->rx_video_sessions[num_video].base.num_inf = num_inf;
            ret = st_json_parse_rx_video(k, video_session,
                                         &ctx->rx_video_sessions[num_video]);
            if (ret) return ret;

            // video output handle id
            ctx->rx_video_sessions[num_video].rx_output_id = rx_output.id;

            num_video++;
          }
        }
      }

      /* parse rx audio sessions */
      json_object* audio_array = st_json_object_object_get(rx_group, "audio");
      if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(audio_array); ++j) {
          json_object* audio_session = json_object_array_get_idx(audio_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(ip_p),
                      ctx->rx_audio_sessions[num_audio].base.ip[0]);
            ctx->rx_audio_sessions[num_audio].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(ip_r),
                        ctx->rx_audio_sessions[num_audio].base.ip[1]);
              ctx->rx_audio_sessions[num_audio].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->rx_audio_sessions[num_audio].base.num_inf = num_inf;
            ret = st_json_parse_rx_audio(k, audio_session,
                                         &ctx->rx_audio_sessions[num_audio]);
            if (ret) return ret;

            // video output handle id
            ctx->rx_audio_sessions[num_audio].rx_output_id = rx_output.id;

            num_audio++;
          }
        }
      }

      /* parse rx ancillary sessions */
      json_object* anc_array = st_json_object_object_get(rx_group, "ancillary");
      if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(anc_array); ++j) {
          json_object* anc_session = json_object_array_get_idx(anc_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(ip_p),
                      ctx->rx_anc_sessions[num_anc].base.ip[0]);
            ctx->rx_anc_sessions[num_anc].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(ip_r),
                        ctx->rx_anc_sessions[num_anc].base.ip[1]);
              ctx->rx_anc_sessions[num_anc].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->rx_anc_sessions[num_anc].base.num_inf = num_inf;
            ret = st_json_parse_rx_anc(k, anc_session, &ctx->rx_anc_sessions[num_anc]);
            if (ret) return ret;
            num_anc++;
          }
        }
      }

      /* parse rx st22p sessions */
      json_object* st22p_array = st_json_object_object_get(rx_group, "st22p");
      if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
          json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(ip_p),
                      ctx->rx_st22p_sessions[num_st22p].base.ip[0]);
            ctx->rx_st22p_sessions[num_st22p].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(ip_r),
                        ctx->rx_st22p_sessions[num_st22p].base.ip[1]);
              ctx->rx_st22p_sessions[num_st22p].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->rx_st22p_sessions[num_st22p].base.num_inf = num_inf;
            ret = st_json_parse_rx_st22p(k, st22p_session,
                                         &ctx->rx_st22p_sessions[num_st22p]);
            if (ret) return ret;
            num_st22p++;
          }
        }
      }

      /* parse rx st20p sessions */
      json_object* st20p_array = st_json_object_object_get(rx_group, "st20p");
      if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
        for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
          json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
          int replicas =
              json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
          if (replicas < 0) {
            logger->error("{}, invalid replicas number: {}", __func__, replicas);
            return -ST_JSON_NOT_VALID;
          }
          for (int k = 0; k < replicas; ++k) {
            inet_pton(AF_INET, json_object_get_string(ip_p),
                      ctx->rx_st20p_sessions[num_st20p].base.ip[0]);
            ctx->rx_st20p_sessions[num_st20p].base.inf[0] = ctx->interfaces[inf_p];
            if (num_inf == 2) {
              inet_pton(AF_INET, json_object_get_string(ip_r),
                        ctx->rx_st20p_sessions[num_st20p].base.ip[1]);
              ctx->rx_st20p_sessions[num_st20p].base.inf[1] = ctx->interfaces[inf_r];
            }
            ctx->rx_st20p_sessions[num_st20p].base.num_inf = num_inf;
            ret = st_json_parse_rx_st20p(k, st20p_session,
                                         &ctx->rx_st20p_sessions[num_st20p]);
            if (ret) return ret;
            num_st20p++;
          }
        }
      }
    }
  }

  return ret;
}

int st_app_parse_json(st_json_context_t* ctx, json_object *root_object) {
  int ret = ST_JSON_SUCCESS;
  if ((ret = st_app_parse_json_sch(ctx, root_object))) return ret;
  if ((ret = st_app_parse_json_interfaces(ctx, root_object))) return ret;
  if ((ret = st_app_parse_json_tx_sessions(ctx, root_object))) return ret;
  if ((ret = st_app_parse_json_rx_sessions(ctx, root_object))) return ret;
  return ret;
}

std::unique_ptr<st_json_context_t> prepare_new_json_context(st_json_context_t* ctx) {
  auto new_ctx = std::unique_ptr<st_json_context_t>(new st_json_context_t {});
  new_ctx->interfaces = ctx->interfaces;
  new_ctx->sch_quota = ctx->sch_quota;
  return new_ctx;
}

template<class K, class V>
Json::Value make_fmt_item(K &&name, V &&value) {
  Json::Value item;
  item["name"] = std::forward<K>(name);
  item["value"] = std::forward<V>(value);
  return item;
}

void set_json_root(st_json_context_t* ctx, json_object *object) {
  const char *json_str = json_object_to_json_string(object);
  Json::Reader reader;
  reader.parse(json_str, ctx->json_root);
}

} // namespace

int st_app_parse_json(st_json_context_t* ctx, const char* filename) {
  //logger->info("{}, using json-c version: {}", __func__, json_c_version());

  std::unique_ptr<json_object, JsonObjectDeleter> root_object { json_object_from_file(filename) };
  if (root_object == NULL) {
    logger->error("{}, can not parse json file {}, please check the format", __func__, filename);
    return -ST_JSON_PARSE_FAIL;
  }
  int ret = st_app_parse_json(ctx, root_object.get());
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


int st_app_parse_json_add(st_json_context_t* ctx, const std::string &json, std::unique_ptr<st_json_context_t> &new_ctx) {
  new_ctx = prepare_new_json_context(ctx);
  auto root = st_app_parse_json_get_object(json);
  int ret = ST_JSON_SUCCESS;
  if ((ret = st_app_parse_json_tx_sessions(new_ctx.get(), root.get()))) return ret;
  if ((ret = st_app_parse_json_rx_sessions(new_ctx.get(), root.get()))) return ret;
  set_json_root(ctx, root.get());
  return ret;
}


int st_app_parse_json_update(st_json_context_t* ctx, const std::string &json, std::unique_ptr<st_json_context_t> &new_ctx) {
  new_ctx = prepare_new_json_context(ctx);
  auto root = st_app_parse_json_get_object(json);
  int ret = ST_JSON_SUCCESS;
  if ((ret = st_app_parse_json_tx_sessions(new_ctx.get(), root.get()))) return ret;
  if ((ret = st_app_parse_json_rx_sessions(new_ctx.get(), root.get()))) return ret;
  set_json_root(ctx, root.get());
  return ret;
}

Json::Value st_app_get_fmts(st_json_context_t* ctx) {
  Json::Value root;

  for (int i = 0; i < ctx->interfaces.size(); ++i) {
    root["tx_sessions.interface"].append(make_fmt_item(ctx->interfaces[i].name, i));
  }

  root["tx_sessions.source.type"].append(make_fmt_item("decklink", "decklink"));
  auto &tx_source_video_format = root["tx_sessions.source.video_format"];
  for (auto &desc : seeder::core::format_descs) {
    tx_source_video_format.append(make_fmt_item(desc.name, desc.name));
  }

  root["tx_sessions.video.type"].append(make_fmt_item("frame", "frame"));
  root["tx_sessions.video.type"].append(make_fmt_item("rtp", "rtp"));
  root["tx_sessions.video.type"].append(make_fmt_item("slice", "slice"));

  root["tx_sessions.video.pacing"].append(make_fmt_item("gap", "gap"));
  root["tx_sessions.video.pacing"].append(make_fmt_item("linear", "linear"));

  root["tx_sessions.video.packing"].append(make_fmt_item("GPM_SL", "GPM_SL"));
  root["tx_sessions.video.packing"].append(make_fmt_item("BPM", "BPM"));
  root["tx_sessions.video.packing"].append(make_fmt_item("GPM", "GPM"));

  root["tx_sessions.video.tr_offset"].append(make_fmt_item("default", "default"));
  root["tx_sessions.video.tr_offset"].append(make_fmt_item("none", "none"));

  for (int i = 0; i < ARRAY_SIZE(st_video_fmt_descs); i++) {
    root["tx_sessions.video.video_format"].append(make_fmt_item(st_video_fmt_descs[i].name, st_video_fmt_descs[i].name));
  }

  root["tx_sessions.video.pg_format"].append(make_fmt_item("YUV_422_10bit", "YUV_422_10bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("YUV_422_8bit", "YUV_422_8bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("YUV_422_12bit", "YUV_422_12bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("YUV_422_16bit", "YUV_422_16bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("YUV_420_8bit", "YUV_420_8bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("YUV_420_10bit", "YUV_420_10bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("YUV_420_12bit", "YUV_420_12bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("RGB_8bit", "RGB_8bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("RGB_10bit", "RGB_10bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("RGB_12bit", "RGB_12bit"));
  root["tx_sessions.video.pg_format"].append(make_fmt_item("RGB_16bit", "RGB_16bit"));

  root["tx_sessions.audio.type"].append(make_fmt_item("frame", "frame"));
  root["tx_sessions.audio.type"].append(make_fmt_item("rtp", "rtp"));

  root["tx_sessions.audio.audio_format"].append(make_fmt_item("PCM8", "PCM8"));
  root["tx_sessions.audio.audio_format"].append(make_fmt_item("PCM16", "PCM16"));
  root["tx_sessions.audio.audio_format"].append(make_fmt_item("PCM24", "PCM24"));
  root["tx_sessions.audio.audio_format"].append(make_fmt_item("AM824", "AM824"));

  // TODO audio_channel 的所有可能组合？
  root["tx_sessions.audio.audio_channel"].append(make_fmt_item("M", "M"));
  root["tx_sessions.audio.audio_channel"].append(make_fmt_item("ST", "ST"));
  root["tx_sessions.audio.audio_channel"].append(make_fmt_item("51", "51"));

  root["tx_sessions.audio.audio_sampling"].append(make_fmt_item("48kHz", "48kHz"));
  root["tx_sessions.audio.audio_sampling"].append(make_fmt_item("96kHz", "96kHz"));
  root["tx_sessions.audio.audio_sampling"].append(make_fmt_item("44.1kHz", "44.1kHz"));

  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("1", "1"));
  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("0.12", "0.12"));
  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("0.25", "0.25"));
  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("0.33", "0.33"));
  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("4", "4"));
  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("0.08", "0.08"));
  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("1.09", "1.09"));
  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("0.14", "0.14"));
  root["tx_sessions.audio.audio_ptime"].append(make_fmt_item("0.09", "0.09"));

  root["tx_sessions.ancillary.type"].append(make_fmt_item("frame", "frame"));
  root["tx_sessions.ancillary.type"].append(make_fmt_item("rtp", "rtp"));

  root["tx_sessions.ancillary.ancillary_format"].append(make_fmt_item("closed_caption", "closed_caption"));

  root["tx_sessions.ancillary.ancillary_fps"].append(make_fmt_item("p59", "p59"));
  root["tx_sessions.ancillary.ancillary_fps"].append(make_fmt_item("p50", "p50"));
  root["tx_sessions.ancillary.ancillary_fps"].append(make_fmt_item("p25", "p25"));
  root["tx_sessions.ancillary.ancillary_fps"].append(make_fmt_item("p29", "p29"));


  return root;
}

// int st_app_parse_json_object_add_tx(st_json_context_t* ctx, json_object* tx_group_array,json_object* root_object,int count)
// {
//   int ret;
//   json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
//   if (interfaces_array == NULL ||
//       json_object_get_type(interfaces_array) != json_type_array) {
//     logger->error("{}, can not parse interfaces", __func__);
//     ret = -ST_JSON_PARSE_FAIL;
//     return error(ret, ctx, root_object);
//   }
//   int num_interfaces = json_object_array_length(interfaces_array);
//   ctx->interfaces = 
//       (st_json_interface_t*)st_app_zmalloc(num_interfaces * sizeof(st_json_interface_t));
//   if (!ctx->interfaces) {
//     logger->error("{}, failed to allocate interfaces", __func__);
//     ret = -ST_JSON_NULL;
//     return error(ret, ctx, root_object);
//   }
//   for (int i = 0; i < num_interfaces; ++i) {
//     ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
//                                    &ctx->interfaces[i]);
//     if (ret) return error(ret, ctx, root_object);
//   }
//   ctx->num_interfaces = num_interfaces;
  
//   if (tx_group_array != NULL && json_object_get_type(tx_group_array) == json_type_array) {
//     /* allocate tx source config*/
//     ctx->tx_source_cnt = json_object_array_length(tx_group_array);
//     ctx->tx_sources = (struct st_app_tx_source*)st_app_zmalloc(ctx->tx_source_cnt*sizeof(struct st_app_tx_source));

//     /* parse session numbers for array allocation */
//     for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
//       json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
//       if (tx_group == NULL) {
//         logger->error("{}, can not parse tx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }
//       int num = 0;
//       /* parse tx video sessions */
//       num = parse_session_num(tx_group, "video");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_video_session_cnt += num;
//       /* parse tx audio sessions */
//       num = parse_session_num(tx_group, "audio");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_audio_session_cnt += num;
//       /* parse tx ancillary sessions */
//       num = parse_session_num(tx_group, "ancillary");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_anc_session_cnt += num;
//       /* parse tx st22p sessions */
//       num = parse_session_num(tx_group, "st22p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_st22p_session_cnt += num;
//       /* parse tx st20p sessions */
//       num = parse_session_num(tx_group, "st20p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_st20p_session_cnt += num;

//       // parse tx source config
//       json_object* source = st_json_object_object_get(tx_group, "source");
//       ctx->tx_sources[i].type = safe_get_string(source, "type"));
//       ctx->tx_sources[i].device_id = json_object_get_int(st_json_object_object_get(source, "device_id"));
//       ctx->tx_sources[i].file_url = safe_get_string(source, "file_url"));
//       ctx->tx_sources[i].video_format = safe_get_string(source, "video_format"));
//     }
    
//     /* allocate tx sessions */
//     ctx->tx_video_sessions = (st_json_video_session_t*)st_app_zmalloc(
//         ctx->tx_video_session_cnt * sizeof(st_json_video_session_t));
//     if (!ctx->tx_video_sessions) {
//       logger->error("{}, failed to allocate tx_video_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_audio_sessions = (st_json_audio_session_t*)st_app_zmalloc(
//         ctx->tx_audio_session_cnt * sizeof(st_json_audio_session_t));
//     if (!ctx->tx_audio_sessions) {
//       logger->error("{}, failed to allocate tx_audio_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_anc_sessions = (st_json_ancillary_session_t*)st_app_zmalloc(
//         ctx->tx_anc_session_cnt * sizeof(st_json_ancillary_session_t));
//     if (!ctx->tx_anc_sessions) {
//       logger->error("{}, failed to allocate tx_anc_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_st22p_sessions = (st_json_st22p_session_t*)st_app_zmalloc(
//         ctx->tx_st22p_session_cnt * sizeof(st_json_st22p_session_t));
//     if (!ctx->tx_st22p_sessions) {
//       logger->error("{}, failed to allocate tx_st22p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_st20p_sessions = (st_json_st20p_session_t*)st_app_zmalloc(
//         ctx->tx_st20p_session_cnt * sizeof(st_json_st20p_session_t));
//     if (!ctx->tx_st20p_sessions) {
//       logger->error("{}, failed to allocate tx_st20p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }

//     int num_inf = 0;
//     int num_video = 0;
//     int num_audio = 0;
//     int num_anc = 0;
//     int num_st22p = 0;
//     int num_st20p = 0;


//     for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
//       json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
//       if (tx_group == NULL) {
//         logger->error("{}, can not parse tx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse destination ip */
//       json_object* dip_p = NULL;
//       json_object* dip_r = NULL;
//       json_object* dip_array = st_json_object_object_get(tx_group, "dip");
//       if (dip_array != NULL && json_object_get_type(dip_array) == json_type_array) {
//         int len = json_object_array_length(dip_array);
//         if (len < 1 || len > MTL_PORT_MAX) {
//           logger->error("{}, wrong dip number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         dip_p = json_object_array_get_idx(dip_array, 0);
//         if (len == 2) {
//           dip_r = json_object_array_get_idx(dip_array, 1);
//         }
//         num_inf = len;
//       } else {
//         logger->error("{}, can not parse dip_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse interface */
//       int inf_p, inf_r = 0;
//       json_object* interface_array = st_json_object_object_get(tx_group, "interface");
//       if (interface_array != NULL &&
//           json_object_get_type(interface_array) == json_type_array) {
//         int len = json_object_array_length(interface_array);
//         if (len != num_inf) {
//           logger->error("{}, wrong interface number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
//         if (inf_p < 0 || inf_p > num_interfaces) {
//           logger->error("{}, wrong interface index", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         if (len == 2) {
//           inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
//           if (inf_r < 0 || inf_r > num_interfaces) {
//             logger->error("{}, wrong interface index", __func__);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//         }
//       } else {
//         logger->error("{}, can not parse interface_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse tx video sessions */
//       json_object* video_array = st_json_object_object_get(tx_group, "video");
//       if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(video_array); ++j) {
//           json_object* video_session = json_object_array_get_idx(video_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(video_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_video_sessions[num_video].base.ip[0]);
//             ctx->tx_video_sessions[num_video].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_video_sessions[num_video].base.ip[1]);
//               ctx->tx_video_sessions[num_video].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_video_sessions[num_video].base.num_inf = num_inf;
//             ret = st_json_parse_tx_video(k, video_session,
//                                          &ctx->tx_video_sessions[num_video]);
//             if (ret) return error(ret, ctx, root_object);

//             // video source handle id
//             ctx->tx_video_sessions[num_video].tx_source_id = i+count;
//             num_video++;
//           }
//         }
//       }

//       /* parse tx audio sessions */
//       json_object* audio_array = st_json_object_object_get(tx_group, "audio");
//       if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(audio_array); ++j) {
//           json_object* audio_session = json_object_array_get_idx(audio_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_audio_sessions[num_audio].base.ip[0]);
//             ctx->tx_audio_sessions[num_audio].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_audio_sessions[num_audio].base.ip[1]);
//               ctx->tx_audio_sessions[num_audio].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_audio_sessions[num_audio].base.num_inf = num_inf;
//             ret = st_json_parse_tx_audio(k, audio_session,
//                                          &ctx->tx_audio_sessions[num_audio]);
//             if (ret) return error(ret, ctx, root_object);

//             // audio source handle id
//             ctx->tx_audio_sessions[num_audio].tx_source_id = i+count;

//             num_audio++;
//           }
//         }
//       }

//       /* parse tx ancillary sessions */
//       json_object* anc_array = st_json_object_object_get(tx_group, "ancillary");
//       if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(anc_array); ++j) {
//           json_object* anc_session = json_object_array_get_idx(anc_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_anc_sessions[num_anc].base.ip[0]);
//             ctx->tx_anc_sessions[num_anc].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_anc_sessions[num_anc].base.ip[1]);
//               ctx->tx_anc_sessions[num_anc].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_anc_sessions[num_anc].base.num_inf = num_inf;
//             ret = st_json_parse_tx_anc(k, anc_session, &ctx->tx_anc_sessions[num_anc]);
//             if (ret) return error(ret, ctx, root_object);
//             num_anc++;
//           }
//         }
//       }

//       /* parse tx st22p sessions */
//       json_object* st22p_array = st_json_object_object_get(tx_group, "st22p");
//       if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
//           json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_st22p_sessions[num_st22p].base.ip[0]);
//             ctx->tx_st22p_sessions[num_st22p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_st22p_sessions[num_st22p].base.ip[1]);
//               ctx->tx_st22p_sessions[num_st22p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_st22p_sessions[num_st22p].base.num_inf = num_inf;
//             ret = st_json_parse_tx_st22p(k, st22p_session,
//                                          &ctx->tx_st22p_sessions[num_st22p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st22p++;
//           }
//         }
//       }

//       /* parse tx st20p sessions */
//       json_object* st20p_array = st_json_object_object_get(tx_group, "st20p");
//       if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
//           json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_st20p_sessions[num_st20p].base.ip[0]);
//             ctx->tx_st20p_sessions[num_st20p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_st20p_sessions[num_st20p].base.ip[1]);
//               ctx->tx_st20p_sessions[num_st20p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_st20p_sessions[num_st20p].base.num_inf = num_inf;
//             ret = st_json_parse_tx_st20p(k, st20p_session,
//                                          &ctx->tx_st20p_sessions[num_st20p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st20p++;
//           }
//         }
//       }
//     }
//   }
//   return ST_JSON_SUCCESS;
// }


// int st_app_parse_json_object_update_tx(st_json_context_t* ctx, json_object*tx_group_array,json_object* root_object)
// {
//   int ret;
//   json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
//   if (interfaces_array == NULL ||
//       json_object_get_type(interfaces_array) != json_type_array) {
//     logger->error("{}, can not parse interfaces", __func__);
//     ret = -ST_JSON_PARSE_FAIL;
//     return error(ret, ctx, root_object);
//   }
//   int num_interfaces = json_object_array_length(interfaces_array);
//   ctx->interfaces = 
//       (st_json_interface_t*)st_app_zmalloc(num_interfaces * sizeof(st_json_interface_t));
//   if (!ctx->interfaces) {
//     logger->error("{}, failed to allocate interfaces", __func__);
//     ret = -ST_JSON_NULL;
//     return error(ret, ctx, root_object);
//   }
//   for (int i = 0; i < num_interfaces; ++i) {
//     ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
//                                    &ctx->interfaces[i]);
//     if (ret) return error(ret, ctx, root_object);
//   }
//   ctx->num_interfaces = num_interfaces;
  
//   if (tx_group_array != NULL && json_object_get_type(tx_group_array) == json_type_array) {
//     /* allocate tx source config*/
//     ctx->tx_source_cnt = json_object_array_length(tx_group_array);
//     ctx->tx_sources = (struct st_app_tx_source*)st_app_zmalloc(ctx->tx_source_cnt*sizeof(struct st_app_tx_source));

//     /* parse session numbers for array allocation */
//     for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
//       json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
//       if (tx_group == NULL) {
//         logger->error("{}, can not parse tx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }
//       int num = 0;
//       /* parse tx video sessions */
//       num = parse_session_num(tx_group, "video");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_video_session_cnt += num;
//       /* parse tx audio sessions */
//       num = parse_session_num(tx_group, "audio");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_audio_session_cnt += num;
//       /* parse tx ancillary sessions */
//       num = parse_session_num(tx_group, "ancillary");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_anc_session_cnt += num;
//       /* parse tx st22p sessions */
//       num = parse_session_num(tx_group, "st22p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_st22p_session_cnt += num;
//       /* parse tx st20p sessions */
//       num = parse_session_num(tx_group, "st20p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->tx_st20p_session_cnt += num;

//       // parse tx source config
//       json_object* source = st_json_object_object_get(tx_group, "source");
//       ctx->tx_sources[i].type = safe_get_string(source, "type");
//       ctx->tx_sources[i].device_id = json_object_get_int(st_json_object_object_get(source, "device_id"));
//       ctx->tx_sources[i].file_url = safe_get_string(source, "file_url");
//       ctx->tx_sources[i].video_format = safe_get_string(source, "video_format");
//     }
    
//     /* allocate tx sessions */
//     ctx->tx_video_sessions = (st_json_video_session_t*)st_app_zmalloc(
//         ctx->tx_video_session_cnt * sizeof(st_json_video_session_t));
//     if (!ctx->tx_video_sessions) {
//       logger->error("{}, failed to allocate tx_video_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_audio_sessions = (st_json_audio_session_t*)st_app_zmalloc(
//         ctx->tx_audio_session_cnt * sizeof(st_json_audio_session_t));
//     if (!ctx->tx_audio_sessions) {
//       logger->error("{}, failed to allocate tx_audio_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_anc_sessions = (st_json_ancillary_session_t*)st_app_zmalloc(
//         ctx->tx_anc_session_cnt * sizeof(st_json_ancillary_session_t));
//     if (!ctx->tx_anc_sessions) {
//       logger->error("{}, failed to allocate tx_anc_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_st22p_sessions = (st_json_st22p_session_t*)st_app_zmalloc(
//         ctx->tx_st22p_session_cnt * sizeof(st_json_st22p_session_t));
//     if (!ctx->tx_st22p_sessions) {
//       logger->error("{}, failed to allocate tx_st22p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->tx_st20p_sessions = (st_json_st20p_session_t*)st_app_zmalloc(
//         ctx->tx_st20p_session_cnt * sizeof(st_json_st20p_session_t));
//     if (!ctx->tx_st20p_sessions) {
//       logger->error("{}, failed to allocate tx_st20p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }

//     int num_inf = 0;
//     int num_video = 0;
//     int num_audio = 0;
//     int num_anc = 0;
//     int num_st22p = 0;
//     int num_st20p = 0;


//     for (int i = 0; i < json_object_array_length(tx_group_array); ++i) {
//       json_object* tx_group = json_object_array_get_idx(tx_group_array, i);
//       int id = json_object_get_int(st_json_object_object_get(tx_group, "id"));
//       if (tx_group == NULL) {
//         logger->error("{}, can not parse tx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse destination ip */
//       json_object* dip_p = NULL;
//       json_object* dip_r = NULL;
//       json_object* dip_array = st_json_object_object_get(tx_group, "dip");
//       if (dip_array != NULL && json_object_get_type(dip_array) == json_type_array) {
//         int len = json_object_array_length(dip_array);
//         if (len < 1 || len > MTL_PORT_MAX) {
//           logger->error("{}, wrong dip number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         dip_p = json_object_array_get_idx(dip_array, 0);
//         if (len == 2) {
//           dip_r = json_object_array_get_idx(dip_array, 1);
//         }
//         num_inf = len;
//       } else {
//         logger->error("{}, can not parse dip_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse interface */
//       int inf_p, inf_r = 0;
//       json_object* interface_array = st_json_object_object_get(tx_group, "interface");
//       if (interface_array != NULL &&
//           json_object_get_type(interface_array) == json_type_array) {
//         int len = json_object_array_length(interface_array);
//         if (len != num_inf) {
//           logger->error("{}, wrong interface number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
//         if (inf_p < 0 || inf_p > num_interfaces) {
//           logger->error("{}, wrong interface index", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         if (len == 2) {
//           inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
//           if (inf_r < 0 || inf_r > num_interfaces) {
//             logger->error("{}, wrong interface index", __func__);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//         }
//       } else {
//         logger->error("{}, can not parse interface_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }


//       /* parse tx video sessions */
//       json_object* video_array = st_json_object_object_get(tx_group, "video");
//       if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(video_array); ++j) {
//           json_object* video_session = json_object_array_get_idx(video_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(video_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_video_sessions[num_video].base.ip[0]);
//             ctx->tx_video_sessions[num_video].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_video_sessions[num_video].base.ip[1]);
//               ctx->tx_video_sessions[num_video].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_video_sessions[num_video].base.num_inf = num_inf;
//             ret = st_json_parse_tx_video(k, video_session,
//                                          &ctx->tx_video_sessions[num_video]);
//             if (ret) return error(ret, ctx, root_object);

//             // video source handle id
//             ctx->tx_video_sessions[num_video].tx_source_id = id;
//             num_video++;
//           }
//         }
//       }

//       /* parse tx audio sessions */
//       json_object* audio_array = st_json_object_object_get(tx_group, "audio");
//       if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(audio_array); ++j) {
//           json_object* audio_session = json_object_array_get_idx(audio_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_audio_sessions[num_audio].base.ip[0]);
//             ctx->tx_audio_sessions[num_audio].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_audio_sessions[num_audio].base.ip[1]);
//               ctx->tx_audio_sessions[num_audio].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_audio_sessions[num_audio].base.num_inf = num_inf;
//             ret = st_json_parse_tx_audio(k, audio_session,
//                                          &ctx->tx_audio_sessions[num_audio]);
//             if (ret) return error(ret, ctx, root_object);

//             // audio source handle id
//             ctx->tx_audio_sessions[num_audio].tx_source_id = id;

//             num_audio++;
//           }
//         }
//       }

//       /* parse tx ancillary sessions */
//       json_object* anc_array = st_json_object_object_get(tx_group, "ancillary");
//       if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(anc_array); ++j) {
//           json_object* anc_session = json_object_array_get_idx(anc_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_anc_sessions[num_anc].base.ip[0]);
//             ctx->tx_anc_sessions[num_anc].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_anc_sessions[num_anc].base.ip[1]);
//               ctx->tx_anc_sessions[num_anc].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_anc_sessions[num_anc].base.num_inf = num_inf;
//             ret = st_json_parse_tx_anc(k, anc_session, &ctx->tx_anc_sessions[num_anc]);
//             if (ret) return error(ret, ctx, root_object);
//             num_anc++;
//           }
//         }
//       }


//       /* parse tx st22p sessions */
//       json_object* st22p_array = st_json_object_object_get(tx_group, "st22p");
//       if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
//           json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_st22p_sessions[num_st22p].base.ip[0]);
//             ctx->tx_st22p_sessions[num_st22p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_st22p_sessions[num_st22p].base.ip[1]);
//               ctx->tx_st22p_sessions[num_st22p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_st22p_sessions[num_st22p].base.num_inf = num_inf;
//             ret = st_json_parse_tx_st22p(k, st22p_session,
//                                          &ctx->tx_st22p_sessions[num_st22p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st22p++;
//           }
//         }
//       }

//       /* parse tx st20p sessions */
//       json_object* st20p_array = st_json_object_object_get(tx_group, "st20p");
//       if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
//           json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(dip_p),
//                       ctx->tx_st20p_sessions[num_st20p].base.ip[0]);
//             ctx->tx_st20p_sessions[num_st20p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(dip_r),
//                         ctx->tx_st20p_sessions[num_st20p].base.ip[1]);
//               ctx->tx_st20p_sessions[num_st20p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->tx_st20p_sessions[num_st20p].base.num_inf = num_inf;
//             ret = st_json_parse_tx_st20p(k, st20p_session,
//                                          &ctx->tx_st20p_sessions[num_st20p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st20p++;
//           }
//         }
//       }
//     }
// }
// return ST_JSON_SUCCESS;
// }


// int st_app_parse_json_object_add_rx(st_json_context_t* ctx, json_object* rx_group_array,json_object* root_object,int count)
// {
//   int ret;
//   /* parse interfaces for system */
//   json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
//   if (interfaces_array == NULL ||
//       json_object_get_type(interfaces_array) != json_type_array) {
//     logger->error("{}, can not parse interfaces", __func__);
//     ret = -ST_JSON_PARSE_FAIL;
//     return error(ret, ctx, root_object);
//   }
//   int num_interfaces = json_object_array_length(interfaces_array);
//   ctx->interfaces = 
//       (st_json_interface_t*)st_app_zmalloc(num_interfaces * sizeof(st_json_interface_t));
//   if (!ctx->interfaces) {
//     logger->error("{}, failed to allocate interfaces", __func__);
//     ret = -ST_JSON_NULL;
//     return error(ret, ctx, root_object);
//   }
//   for (int i = 0; i < num_interfaces; ++i) {
//     ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
//                                    &ctx->interfaces[i]);
//     if (ret) return error(ret, ctx, root_object);
//   }
//   ctx->num_interfaces = num_interfaces;

//     if (rx_group_array != NULL && json_object_get_type(rx_group_array) == json_type_array) {
//     /* allocate rx output config*/
//     ctx->rx_output_cnt = json_object_array_length(rx_group_array);
//     ctx->rx_output = (st_app_rx_output*)st_app_zmalloc(ctx->rx_output_cnt*sizeof(st_app_rx_output));

//     /* parse session numbers for array allocation */
//     for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
//       json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
//       if (rx_group == NULL) {
//         logger->error("{}, can not parse rx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }
//       int num = 0;
//       /* parse rx video sessions */
//       num = parse_session_num(rx_group, "video");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_video_session_cnt += num;
//       /* parse rx audio sessions */
//       num = parse_session_num(rx_group, "audio");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_audio_session_cnt += num;
//       /* parse rx ancillary sessions */
//       num = parse_session_num(rx_group, "ancillary");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_anc_session_cnt += num;
//       /* parse rx st22p sessions */
//       num = parse_session_num(rx_group, "st22p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_st22p_session_cnt += num;
//       /* parse rx st20p sessions */
//       num = parse_session_num(rx_group, "st20p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_st20p_session_cnt += num;

//       // parse rx output config
//       json_object* output = st_json_object_object_get(rx_group, "output");
//       ctx->rx_output[i].type = safe_get_string(output, "type"));
//       ctx->rx_output[i].device_id = json_object_get_int(st_json_object_object_get(output, "device_id"));
//       std::string fu = safe_get_string(output, "file_url"));
//       if(!(fu.empty())) ctx->rx_output[i].file_url = fu;
//       ctx->rx_output[i].video_format = safe_get_string(output, "video_format"));
//     }

//     /* allocate tx sessions */
//     ctx->rx_video_sessions = (st_json_video_session_t*)st_app_zmalloc(
//         ctx->rx_video_session_cnt * sizeof(st_json_video_session_t));
//     if (!ctx->rx_video_sessions) {
//       logger->error("{}, failed to allocate rx_video_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_audio_sessions = (st_json_audio_session_t*)st_app_zmalloc(
//         ctx->rx_audio_session_cnt * sizeof(st_json_audio_session_t));
//     if (!ctx->rx_audio_sessions) {
//       logger->error("{}, failed to allocate rx_audio_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_anc_sessions = (st_json_ancillary_session_t*)st_app_zmalloc(
//         ctx->rx_anc_session_cnt * sizeof(st_json_ancillary_session_t));
//     if (!ctx->rx_anc_sessions) {
//       logger->error("{}, failed to allocate rx_anc_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_st22p_sessions = (st_json_st22p_session_t*)st_app_zmalloc(
//         ctx->rx_st22p_session_cnt * sizeof(st_json_st22p_session_t));
//     if (!ctx->rx_st22p_sessions) {
//       logger->error("{}, failed to allocate rx_st22p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_st20p_sessions = (st_json_st20p_session_t*)st_app_zmalloc(
//         ctx->rx_st20p_session_cnt * sizeof(st_json_st20p_session_t));
//     if (!ctx->rx_st20p_sessions) {
//       logger->error("{}, failed to allocate rx_st20p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }

//     int num_inf = 0;
//     int num_video = 0;
//     int num_audio = 0;
//     int num_anc = 0;
//     int num_st22p = 0;
//     int num_st20p = 0;

//     for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
//       json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
//       if (rx_group == NULL) {
//         logger->error("{}, can not parse rx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse receiving ip */
//       json_object* ip_p = NULL;
//       json_object* ip_r = NULL;
//       json_object* ip_array = st_json_object_object_get(rx_group, "ip");
//       if (ip_array != NULL && json_object_get_type(ip_array) == json_type_array) {
//         int len = json_object_array_length(ip_array);
//         if (len < 1 || len > MTL_PORT_MAX) {
//           logger->error("{}, wrong dip number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         ip_p = json_object_array_get_idx(ip_array, 0);
//         if (len == 2) {
//           ip_r = json_object_array_get_idx(ip_array, 1);
//         }
//         num_inf = len;
//       } else {
//         logger->error("{}, can not parse dip_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse interface */
//       int inf_p, inf_r = 0;
//       json_object* interface_array = st_json_object_object_get(rx_group, "interface");
//       if (interface_array != NULL &&
//           json_object_get_type(interface_array) == json_type_array) {
//         int len = json_object_array_length(interface_array);
//         if (len != num_inf) {
//           logger->error("{}, wrong interface number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
//         if (inf_p < 0 || inf_p > num_interfaces) {
//           logger->error("{}, wrong interface index", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         if (len == 2) {
//           inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
//           if (inf_r < 0 || inf_r > num_interfaces) {
//             logger->error("{}, wrong interface index", __func__);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//         }
//       } else {
//         logger->error("{}, can not parse interface_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse rx video sessions */
//       json_object* video_array = st_json_object_object_get(rx_group, "video");
//       if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(video_array); ++j) {
//           json_object* video_session = json_object_array_get_idx(video_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(video_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_video_sessions[num_video].base.ip[0]);
//             ctx->rx_video_sessions[num_video].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_video_sessions[num_video].base.ip[1]);
//               ctx->rx_video_sessions[num_video].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_video_sessions[num_video].base.num_inf = num_inf;
//             ret = st_json_parse_rx_video(k, video_session,
//                                          &ctx->rx_video_sessions[num_video]);
//             if (ret) return error(ret, ctx, root_object);

//             // video output handle id
//             ctx->rx_video_sessions[num_video].rx_output_id = i+count;

//             num_video++;
//           }
//         }
//       }

//       /* parse rx audio sessions */
//       json_object* audio_array = st_json_object_object_get(rx_group, "audio");
//       if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(audio_array); ++j) {
//           json_object* audio_session = json_object_array_get_idx(audio_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_audio_sessions[num_audio].base.ip[0]);
//             ctx->rx_audio_sessions[num_audio].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_audio_sessions[num_audio].base.ip[1]);
//               ctx->rx_audio_sessions[num_audio].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_audio_sessions[num_audio].base.num_inf = num_inf;
//             ret = st_json_parse_rx_audio(k, audio_session,
//                                          &ctx->rx_audio_sessions[num_audio]);
//             if (ret) return error(ret, ctx, root_object);

//             // video output handle id
//             ctx->rx_audio_sessions[num_audio].rx_output_id = i+count;

//             num_audio++;
//           }
//         }
//       }

//       /* parse rx ancillary sessions */
//       json_object* anc_array = st_json_object_object_get(rx_group, "ancillary");
//       if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(anc_array); ++j) {
//           json_object* anc_session = json_object_array_get_idx(anc_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_anc_sessions[num_anc].base.ip[0]);
//             ctx->rx_anc_sessions[num_anc].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_anc_sessions[num_anc].base.ip[1]);
//               ctx->rx_anc_sessions[num_anc].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_anc_sessions[num_anc].base.num_inf = num_inf;
//             ret = st_json_parse_rx_anc(k, anc_session, &ctx->rx_anc_sessions[num_anc]);
//             if (ret) return error(ret, ctx, root_object);
//             num_anc++;
//           }
//         }
//       }

//       /* parse rx st22p sessions */
//       json_object* st22p_array = st_json_object_object_get(rx_group, "st22p");
//       if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
//           json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_st22p_sessions[num_st22p].base.ip[0]);
//             ctx->rx_st22p_sessions[num_st22p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_st22p_sessions[num_st22p].base.ip[1]);
//               ctx->rx_st22p_sessions[num_st22p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_st22p_sessions[num_st22p].base.num_inf = num_inf;
//             ret = st_json_parse_rx_st22p(k, st22p_session,
//                                          &ctx->rx_st22p_sessions[num_st22p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st22p++;
//           }
//         }
//       }

//       /* parse rx st20p sessions */
//       json_object* st20p_array = st_json_object_object_get(rx_group, "st20p");
//       if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
//           json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_st20p_sessions[num_st20p].base.ip[0]);
//             ctx->rx_st20p_sessions[num_st20p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_st20p_sessions[num_st20p].base.ip[1]);
//               ctx->rx_st20p_sessions[num_st20p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_st20p_sessions[num_st20p].base.num_inf = num_inf;
//             ret = st_json_parse_rx_st20p(k, st20p_session,
//                                          &ctx->rx_st20p_sessions[num_st20p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st20p++;
//           }
//         }
//       }
//     }
//   }
//   return ST_JSON_SUCCESS;
// }


// int st_app_parse_json_object_update_rx(st_json_context_t* ctx, json_object* rx_group_array,json_object* root_object)
// {
//   int ret;
//   json_object* interfaces_array = st_json_object_object_get(root_object, "interfaces");
//   if (interfaces_array == NULL ||
//       json_object_get_type(interfaces_array) != json_type_array) {
//     logger->error("{}, can not parse interfaces", __func__);
//     ret = -ST_JSON_PARSE_FAIL;
//     return error(ret, ctx, root_object);
//   }
//   int num_interfaces = json_object_array_length(interfaces_array);
//   ctx->interfaces = 
//       (st_json_interface_t*)st_app_zmalloc(num_interfaces * sizeof(st_json_interface_t));
//   if (!ctx->interfaces) {
//     logger->error("{}, failed to allocate interfaces", __func__);
//     ret = -ST_JSON_NULL;
//     return error(ret, ctx, root_object);
//   }
//   for (int i = 0; i < num_interfaces; ++i) {
//     ret = st_json_parse_interfaces(json_object_array_get_idx(interfaces_array, i),
//                                    &ctx->interfaces[i]);
//     if (ret) return error(ret, ctx, root_object);
//   }
//   ctx->num_interfaces = num_interfaces;

// if (rx_group_array != NULL && json_object_get_type(rx_group_array) == json_type_array) {
//     /* allocate rx output config*/
//     ctx->rx_output_cnt = json_object_array_length(rx_group_array);
//     ctx->rx_output = (st_app_rx_output*)st_app_zmalloc(ctx->rx_output_cnt*sizeof(st_app_rx_output));

//     /* parse session numbers for array allocation */
//     for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
//       json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
//       if (rx_group == NULL) {
//         logger->error("{}, can not parse rx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }
//       int num = 0;
//       /* parse rx video sessions */
//       num = parse_session_num(rx_group, "video");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_video_session_cnt += num;
//       /* parse rx audio sessions */
//       num = parse_session_num(rx_group, "audio");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_audio_session_cnt += num;
//       /* parse rx ancillary sessions */
//       num = parse_session_num(rx_group, "ancillary");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_anc_session_cnt += num;
//       /* parse rx st22p sessions */
//       num = parse_session_num(rx_group, "st22p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_st22p_session_cnt += num;
//       /* parse rx st20p sessions */
//       num = parse_session_num(rx_group, "st20p");
//       if (num < 0) return error(ret, ctx, root_object);
//       ctx->rx_st20p_session_cnt += num;

//       // parse rx output config
//       json_object* output = st_json_object_object_get(rx_group, "output");
//       ctx->rx_output[i].type = safe_get_string(output, "type"));
//       ctx->rx_output[i].device_id = json_object_get_int(st_json_object_object_get(output, "device_id"));
//       std::string fu = safe_get_string(output, "file_url"));
//       if(!(fu.empty())) ctx->rx_output[i].file_url = fu;
//       ctx->rx_output[i].video_format = safe_get_string(output, "video_format"));
//     }

//     /* allocate rx sessions */
//     ctx->rx_video_sessions = (st_json_video_session_t*)st_app_zmalloc(
//         ctx->rx_video_session_cnt * sizeof(st_json_video_session_t));
//     if (!ctx->rx_video_sessions) {
//       logger->error("{}, failed to allocate rx_video_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_audio_sessions = (st_json_audio_session_t*)st_app_zmalloc(
//         ctx->rx_audio_session_cnt * sizeof(st_json_audio_session_t));
//     if (!ctx->rx_audio_sessions) {
//       logger->error("{}, failed to allocate rx_audio_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_anc_sessions = (st_json_ancillary_session_t*)st_app_zmalloc(
//         ctx->rx_anc_session_cnt * sizeof(st_json_ancillary_session_t));
//     if (!ctx->rx_anc_sessions) {
//       logger->error("{}, failed to allocate rx_anc_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_st22p_sessions = (st_json_st22p_session_t*)st_app_zmalloc(
//         ctx->rx_st22p_session_cnt * sizeof(st_json_st22p_session_t));
//     if (!ctx->rx_st22p_sessions) {
//       logger->error("{}, failed to allocate rx_st22p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }
//     ctx->rx_st20p_sessions = (st_json_st20p_session_t*)st_app_zmalloc(
//         ctx->rx_st20p_session_cnt * sizeof(st_json_st20p_session_t));
//     if (!ctx->rx_st20p_sessions) {
//       logger->error("{}, failed to allocate rx_st20p_sessions", __func__);
//       ret = -ST_JSON_NULL;
//       return error(ret, ctx, root_object);
//     }

//     int num_inf = 0;
//     int num_video = 0;
//     int num_audio = 0;
//     int num_anc = 0;
//     int num_st22p = 0;
//     int num_st20p = 0;

//     for (int i = 0; i < json_object_array_length(rx_group_array); ++i) {
//       json_object* rx_group = json_object_array_get_idx(rx_group_array, i);
//       int id = json_object_get_int(st_json_object_object_get(rx_group, "id"));
//       if (rx_group == NULL) {
//         logger->error("{}, can not parse rx session group", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse receiving ip */
//       json_object* ip_p = NULL;
//       json_object* ip_r = NULL;
//       json_object* ip_array = st_json_object_object_get(rx_group, "ip");
//       if (ip_array != NULL && json_object_get_type(ip_array) == json_type_array) {
//         int len = json_object_array_length(ip_array);
//         if (len < 1 || len > MTL_PORT_MAX) {
//           logger->error("{}, wrong dip number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         ip_p = json_object_array_get_idx(ip_array, 0);
//         if (len == 2) {
//           ip_r = json_object_array_get_idx(ip_array, 1);
//         }
//         num_inf = len;
//       } else {
//         logger->error("{}, can not parse dip_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse interface */
//       int inf_p, inf_r = 0;
//       json_object* interface_array = st_json_object_object_get(rx_group, "interface");
//       if (interface_array != NULL &&
//           json_object_get_type(interface_array) == json_type_array) {
//         int len = json_object_array_length(interface_array);
//         if (len != num_inf) {
//           logger->error("{}, wrong interface number", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         inf_p = json_object_get_int(json_object_array_get_idx(interface_array, 0));
//         if (inf_p < 0 || inf_p > num_interfaces) {
//           logger->error("{}, wrong interface index", __func__);
//           ret = -ST_JSON_NOT_VALID;
//           return error(ret, ctx, root_object);
//         }
//         if (len == 2) {
//           inf_r = json_object_get_int(json_object_array_get_idx(interface_array, 1));
//           if (inf_r < 0 || inf_r > num_interfaces) {
//             logger->error("{}, wrong interface index", __func__);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//         }
//       } else {
//         logger->error("{}, can not parse interface_array", __func__);
//         ret = -ST_JSON_PARSE_FAIL;
//         return error(ret, ctx, root_object);
//       }

//       /* parse rx video sessions */
//       json_object* video_array = st_json_object_object_get(rx_group, "video");
//       if (video_array != NULL && json_object_get_type(video_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(video_array); ++j) {
//           json_object* video_session = json_object_array_get_idx(video_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(video_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_video_sessions[num_video].base.ip[0]);
//             ctx->rx_video_sessions[num_video].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_video_sessions[num_video].base.ip[1]);
//               ctx->rx_video_sessions[num_video].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_video_sessions[num_video].base.num_inf = num_inf;
//             ret = st_json_parse_rx_video(k, video_session,
//                                          &ctx->rx_video_sessions[num_video]);
//             if (ret) return error(ret, ctx, root_object);

//             // video output handle id
//             ctx->rx_video_sessions[num_video].rx_output_id = id;

//             num_video++;
//           }
//         }
//       }

//       /* parse rx audio sessions */
//       json_object* audio_array = st_json_object_object_get(rx_group, "audio");
//       if (audio_array != NULL && json_object_get_type(audio_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(audio_array); ++j) {
//           json_object* audio_session = json_object_array_get_idx(audio_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(audio_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_audio_sessions[num_audio].base.ip[0]);
//             ctx->rx_audio_sessions[num_audio].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_audio_sessions[num_audio].base.ip[1]);
//               ctx->rx_audio_sessions[num_audio].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_audio_sessions[num_audio].base.num_inf = num_inf;
//             ret = st_json_parse_rx_audio(k, audio_session,
//                                          &ctx->rx_audio_sessions[num_audio]);
//             if (ret) return error(ret, ctx, root_object);

//             // video output handle id
//             ctx->rx_audio_sessions[num_audio].rx_output_id = id;

//             num_audio++;
//           }
//         }
//       }

//       /* parse rx ancillary sessions */
//       json_object* anc_array = st_json_object_object_get(rx_group, "ancillary");
//       if (anc_array != NULL && json_object_get_type(anc_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(anc_array); ++j) {
//           json_object* anc_session = json_object_array_get_idx(anc_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(anc_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_anc_sessions[num_anc].base.ip[0]);
//             ctx->rx_anc_sessions[num_anc].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_anc_sessions[num_anc].base.ip[1]);
//               ctx->rx_anc_sessions[num_anc].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_anc_sessions[num_anc].base.num_inf = num_inf;
//             ret = st_json_parse_rx_anc(k, anc_session, &ctx->rx_anc_sessions[num_anc]);
//             if (ret) return error(ret, ctx, root_object);
//             num_anc++;
//           }
//         }
//       }

//       /* parse rx st22p sessions */
//       json_object* st22p_array = st_json_object_object_get(rx_group, "st22p");
//       if (st22p_array != NULL && json_object_get_type(st22p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st22p_array); ++j) {
//           json_object* st22p_session = json_object_array_get_idx(st22p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st22p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_st22p_sessions[num_st22p].base.ip[0]);
//             ctx->rx_st22p_sessions[num_st22p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_st22p_sessions[num_st22p].base.ip[1]);
//               ctx->rx_st22p_sessions[num_st22p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_st22p_sessions[num_st22p].base.num_inf = num_inf;
//             ret = st_json_parse_rx_st22p(k, st22p_session,
//                                          &ctx->rx_st22p_sessions[num_st22p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st22p++;
//           }
//         }
//       }

//       /* parse rx st20p sessions */
//       json_object* st20p_array = st_json_object_object_get(rx_group, "st20p");
//       if (st20p_array != NULL && json_object_get_type(st20p_array) == json_type_array) {
//         for (int j = 0; j < json_object_array_length(st20p_array); ++j) {
//           json_object* st20p_session = json_object_array_get_idx(st20p_array, j);
//           int replicas =
//               json_object_get_int(st_json_object_object_get(st20p_session, "replicas"));
//           if (replicas < 0) {
//             logger->error("{}, invalid replicas number: {}", __func__, replicas);
//             ret = -ST_JSON_NOT_VALID;
//             return error(ret, ctx, root_object);
//           }
//           for (int k = 0; k < replicas; ++k) {
//             inet_pton(AF_INET, json_object_get_string(ip_p),
//                       ctx->rx_st20p_sessions[num_st20p].base.ip[0]);
//             ctx->rx_st20p_sessions[num_st20p].base.inf[0] = &ctx->interfaces[inf_p];
//             if (num_inf == 2) {
//               inet_pton(AF_INET, json_object_get_string(ip_r),
//                         ctx->rx_st20p_sessions[num_st20p].base.ip[1]);
//               ctx->rx_st20p_sessions[num_st20p].base.inf[1] = &ctx->interfaces[inf_r];
//             }
//             ctx->rx_st20p_sessions[num_st20p].base.num_inf = num_inf;
//             ret = st_json_parse_rx_st20p(k, st20p_session,
//                                          &ctx->rx_st20p_sessions[num_st20p]);
//             if (ret) return error(ret, ctx, root_object);
//             num_st20p++;
//           }
//         }
//       }
//     }
//   }
//   return ST_JSON_SUCCESS;
// }

st_json_context::~st_json_context() {}