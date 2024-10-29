/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2022 Intel Corporation
 */

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <getopt.h>
#include <inttypes.h>
#include <spdlog/common.h>

// #include <mtl/mtl_seeder_api.h>

#include "app_base.h"
#include "core/util/logger.h"
#include <boost/program_options.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace seeder::core;

enum st_args_cmd {
  ST_ARG_UNKNOWN = 0,

  ST_ARG_P_PORT = 0x100, /* start from end of ascii */
  ST_ARG_R_PORT,
  ST_ARG_P_TX_IP,
  ST_ARG_R_TX_IP,
  ST_ARG_P_RX_IP,
  ST_ARG_R_RX_IP,
  ST_ARG_P_SIP,
  ST_ARG_R_SIP,

  ST_ARG_TX_VIDEO_URL = 0x200,
  ST_ARG_TX_VIDEO_SESSIONS_CNT,
  ST_ARG_TX_VIDEO_RTP_RING_SIZE,
  ST_ARG_TX_AUDIO_URL,
  ST_ARG_TX_AUDIO_SESSIONS_CNT,
  ST_ARG_TX_AUDIO_RTP_RING_SIZE,
  ST_ARG_TX_ANC_URL,
  ST_ARG_TX_ANC_SESSIONS_CNT,
  ST_ARG_TX_ANC_RTP_RING_SIZE,
  ST22_ARG_TX_SESSIONS_CNT,
  ST22_ARG_TX_URL,
  ST_ARG_RX_VIDEO_SESSIONS_CNT,
  ST_ARG_RX_VIDEO_FLIE_FRAMES,
  ST_ARG_RX_VIDEO_FB_CNT,
  ST_ARG_RX_VIDEO_RTP_RING_SIZE,
  ST_ARG_RX_AUDIO_SESSIONS_CNT,
  ST_ARG_RX_AUDIO_RTP_RING_SIZE,
  ST_ARG_RX_ANC_SESSIONS_CNT,
  ST22_ARG_RX_SESSIONS_CNT,
  ST_ARG_HDR_SPLIT,
  ST_ARG_PACING_WAY,

  ST_ARG_CONFIG_FILE = 0x300,
  ST_ARG_TEST_TIME,
  ST_ARG_PTP_UNICAST_ADDR,
  ST_ARG_CNI_THREAD,
  ST_ARG_RX_EBU,
  ST_ARG_USER_LCORES,
  ST_ARG_SCH_DATA_QUOTA,
  ST_ARG_SCH_SESSION_QUOTA,
  ST_ARG_P_TX_DST_MAC,
  ST_ARG_R_TX_DST_MAC,
  ST_ARG_NIC_RX_PROMISCUOUS,
  ST_ARG_RX_VIDEO_DISPLAY,
  ST_ARG_LIB_PTP,
  ST_ARG_PTP_PI,
  ST_ARG_PTP_KP,
  ST_ARG_PTP_KI,
  ST_ARG_PTP_TSC,
  ST_ARG_RX_MONO_POOL,
  ST_ARG_TX_MONO_POOL,
  ST_ARG_MONO_POOL,
  ST_ARG_RX_POOL_DATA_SIZE,
  ST_ARG_LOG_LEVEL,
  ST_ARG_NB_TX_DESC,
  ST_ARG_NB_RX_DESC,
  ST_ARG_DMA_DEV,
  ST_ARG_RX_SEPARATE_VIDEO_LCORE,
  ST_ARG_RX_MIX_VIDEO_LCORE,
  ST_ARG_TSC_PACING,
  ST_ARG_PCAPNG_DUMP,
  ST_ARG_RUNTIME_SESSION,
  ST_ARG_TTF_FILE,
  ST_ARG_AF_XDP_ZC_DISABLE,
  ST_ARG_START_QUEUE,
  ST_ARG_P_START_QUEUE,
  ST_ARG_R_START_QUEUE,
  ST_ARG_TASKLET_TIME,
  ST_ARG_UTC_OFFSET,
  ST_ARG_NO_SYSTEM_RX_QUEUES,
  ST_ARG_TX_COPY_ONCE,
  ST_ARG_TASKLET_THREAD,
  ST_ARG_TASKLET_SLEEP,
  ST_ARG_APP_THREAD,
  ST_ARG_RXTX_SIMD_512,
  ST_ARG_HTTP_SERVER_PORT,
  ST_ARG_TX_SESSIONS_CNT_MAX,
  ST_ARG_RX_SESSIONS_CNT_MAX,
  ST_ARG_DECKLINK_ID_MAP,
  ST_ARG_MTL_DETECT_RETRY,
  ST_ARG_MAX,
};

/*
struct option {
   const char *name;
   int has_arg;
   int *flag;
   int val;
};
*/
static struct option st_app_args_options[] = {
    {"p_port", required_argument, 0, ST_ARG_P_PORT},
    {"r_port", required_argument, 0, ST_ARG_R_PORT},
    {"p_tx_ip", required_argument, 0, ST_ARG_P_TX_IP},
    {"r_tx_ip", required_argument, 0, ST_ARG_R_TX_IP},
    {"p_rx_ip", required_argument, 0, ST_ARG_P_RX_IP},
    {"r_rx_ip", required_argument, 0, ST_ARG_R_RX_IP},
    {"p_sip", required_argument, 0, ST_ARG_P_SIP},
    {"r_sip", required_argument, 0, ST_ARG_R_SIP},

    {"tx_video_url", required_argument, 0, ST_ARG_TX_VIDEO_URL},
    {"tx_video_sessions_count", required_argument, 0, ST_ARG_TX_VIDEO_SESSIONS_CNT},
    {"tx_video_rtp_ring_size", required_argument, 0, ST_ARG_TX_VIDEO_RTP_RING_SIZE},
    {"tx_audio_url", required_argument, 0, ST_ARG_TX_AUDIO_URL},
    {"tx_audio_sessions_count", required_argument, 0, ST_ARG_TX_AUDIO_SESSIONS_CNT},
    {"tx_audio_rtp_ring_size", required_argument, 0, ST_ARG_TX_AUDIO_RTP_RING_SIZE},
    {"tx_anc_url", required_argument, 0, ST_ARG_TX_ANC_URL},
    {"tx_anc_sessions_count", required_argument, 0, ST_ARG_TX_ANC_SESSIONS_CNT},
    {"tx_anc_rtp_ring_size", required_argument, 0, ST_ARG_TX_ANC_RTP_RING_SIZE},
    {"tx_st22_sessions_count", required_argument, 0, ST22_ARG_TX_SESSIONS_CNT},
    {"tx_st22_url", required_argument, 0, ST22_ARG_TX_URL},

    {"rx_video_sessions_count", required_argument, 0, ST_ARG_RX_VIDEO_SESSIONS_CNT},
    {"rx_video_file_frames", required_argument, 0, ST_ARG_RX_VIDEO_FLIE_FRAMES},
    {"rx_video_fb_cnt", required_argument, 0, ST_ARG_RX_VIDEO_FB_CNT},
    {"rx_video_rtp_ring_size", required_argument, 0, ST_ARG_RX_VIDEO_RTP_RING_SIZE},
    {"display", no_argument, 0, ST_ARG_RX_VIDEO_DISPLAY},
    {"rx_audio_sessions_count", required_argument, 0, ST_ARG_RX_AUDIO_SESSIONS_CNT},
    {"rx_audio_rtp_ring_size", required_argument, 0, ST_ARG_RX_AUDIO_RTP_RING_SIZE},
    {"rx_anc_sessions_count", required_argument, 0, ST_ARG_RX_ANC_SESSIONS_CNT},
    {"rx_st22_sessions_count", required_argument, 0, ST22_ARG_RX_SESSIONS_CNT},
    {"hdr_split", no_argument, 0, ST_ARG_HDR_SPLIT},
    {"pacing_way", required_argument, 0, ST_ARG_PACING_WAY},

    {"config_file", required_argument, 0, ST_ARG_CONFIG_FILE},
    {"test_time", required_argument, 0, ST_ARG_TEST_TIME},
    {"ptp_unicast", no_argument, 0, ST_ARG_PTP_UNICAST_ADDR},
    {"cni_thread", no_argument, 0, ST_ARG_CNI_THREAD},
    {"ebu", no_argument, 0, ST_ARG_RX_EBU},
    {"lcores", required_argument, 0, ST_ARG_USER_LCORES},
    {"sch_data_quota", required_argument, 0, ST_ARG_SCH_DATA_QUOTA},
    {"sch_session_quota", required_argument, 0, ST_ARG_SCH_SESSION_QUOTA},
    {"p_tx_dst_mac", required_argument, 0, ST_ARG_P_TX_DST_MAC},
    {"r_tx_dst_mac", required_argument, 0, ST_ARG_R_TX_DST_MAC},
    {"promiscuous", no_argument, 0, ST_ARG_NIC_RX_PROMISCUOUS},
    {"log_level", required_argument, 0, ST_ARG_LOG_LEVEL},
    {"ptp", no_argument, 0, ST_ARG_LIB_PTP},
    {"pi", no_argument, 0, ST_ARG_PTP_PI},
    {"kp", required_argument, 0, ST_ARG_PTP_KP},
    {"ki", required_argument, 0, ST_ARG_PTP_KI},
    {"ptp_tsc", no_argument, 0, ST_ARG_PTP_TSC},
    {"rx_mono_pool", no_argument, 0, ST_ARG_RX_MONO_POOL},
    {"tx_mono_pool", no_argument, 0, ST_ARG_TX_MONO_POOL},
    {"mono_pool", no_argument, 0, ST_ARG_MONO_POOL},
    {"rx_pool_data_size", required_argument, 0, ST_ARG_RX_POOL_DATA_SIZE},
    {"rx_separate_lcore", no_argument, 0, ST_ARG_RX_SEPARATE_VIDEO_LCORE},
    {"rx_mix_lcore", no_argument, 0, ST_ARG_RX_MIX_VIDEO_LCORE},
    {"nb_tx_desc", required_argument, 0, ST_ARG_NB_TX_DESC},
    {"nb_rx_desc", required_argument, 0, ST_ARG_NB_RX_DESC},
    {"dma_dev", required_argument, 0, ST_ARG_DMA_DEV},
    {"tsc", no_argument, 0, ST_ARG_TSC_PACING},
    {"pcapng_dump", required_argument, 0, ST_ARG_PCAPNG_DUMP},
    {"runtime_session", no_argument, 0, ST_ARG_RUNTIME_SESSION},
    {"ttf_file", required_argument, 0, ST_ARG_TTF_FILE},
    {"afxdp_zc_disable", no_argument, 0, ST_ARG_AF_XDP_ZC_DISABLE},
    {"start_queue", required_argument, 0, ST_ARG_START_QUEUE},
    {"p_start_queue", required_argument, 0, ST_ARG_P_START_QUEUE},
    {"r_start_queue", required_argument, 0, ST_ARG_R_START_QUEUE},
    {"tasklet_time", no_argument, 0, ST_ARG_TASKLET_TIME},
    {"utc_offset", required_argument, 0, ST_ARG_UTC_OFFSET},
    {"no_srq", no_argument, 0, ST_ARG_NO_SYSTEM_RX_QUEUES},
    {"tx_copy_once", no_argument, 0, ST_ARG_TX_COPY_ONCE},
    {"tasklet_thread", no_argument, 0, ST_ARG_TASKLET_THREAD},
    {"tasklet_sleep", no_argument, 0, ST_ARG_TASKLET_SLEEP},
    {"app_thread", no_argument, 0, ST_ARG_APP_THREAD},
    {"rxtx_simd_512", no_argument, 0, ST_ARG_RXTX_SIMD_512},
    {"http_port", required_argument, 0, ST_ARG_HTTP_SERVER_PORT},
    {"tx_sessions_cnt_max", required_argument, 0, ST_ARG_TX_SESSIONS_CNT_MAX},
    {"rx_sessions_cnt_max", required_argument, 0, ST_ARG_RX_SESSIONS_CNT_MAX},
    {"decklink_id_map", required_argument, 0, ST_ARG_DECKLINK_ID_MAP},
    {"mtl_detect_retry", required_argument, 0, ST_ARG_MTL_DETECT_RETRY},
    {0, 0, 0, 0}};

static int app_args_parse_lcores(struct mtl_init_params* p, char* list) {
  if (!list) return -EIO;

  logger->debug("{}, lcore list {}", __func__, list);
  p->lcores = list;
  return 0;
}

static int app_args_dma_dev(struct mtl_init_params* p, char* in_dev) {
  if (!in_dev) return -EIO;
  char devs[128] = {0};
  strncpy(devs, in_dev, 128 - 1);

  logger->debug("{}, dev list {}", __func__, devs);
  char* next_dev = strtok(devs, ",");
  while (next_dev && (p->num_dma_dev_port < MTL_DMA_DEV_MAX)) {
    logger->debug("next_dev: {}", next_dev);
    strncpy(p->dma_dev_port[p->num_dma_dev_port], next_dev, MTL_PORT_MAX_LEN - 1);
    p->num_dma_dev_port++;
    next_dev = strtok(NULL, ",");
  }
  return 0;
}

static int app_args_json(struct st_app_context* ctx, struct mtl_init_params* p,
                         char* json_file) {
  ctx->json_ctx = std::shared_ptr<st_json_context_t>(new st_json_context_t {});
  int ret = st_app_parse_json(ctx->json_ctx.get(), json_file).value();
  if (ret < 0) {
    logger->error("{}, st_app_parse_json fail {}", __func__, ret);
    ctx->json_ctx.reset();
    return ret;
  }
  for (auto &inf : ctx->json_ctx->interfaces) {
    if (inf.ip_addr_str.empty()) {
      continue;
    }
    snprintf(p->port[p->num_ports], sizeof(p->port[p->num_ports]), "%s", inf.name);
    memcpy(p->sip_addr[p->num_ports], inf.ip_addr, MTL_IP_ADDR_LEN);
    p->num_ports++;
  }
  if (ctx->json_ctx->sch_quota) {
    p->data_quota_mbs_per_sch =
        ctx->json_ctx->sch_quota * st20_1080p59_yuv422_10bit_bandwidth_mps();
  }
  for (int i = 0; i < ctx->json_ctx->rx_video_sessions.size(); i++) {
    int w = st_app_get_width(ctx->json_ctx->rx_video_sessions[i].info.video_format);
    if (w > ctx->rx_max_width) ctx->rx_max_width = w;
    int h = st_app_get_height(ctx->json_ctx->rx_video_sessions[i].info.video_format);
    if (h > ctx->rx_max_height) ctx->rx_max_height = h;
  }
  return 0;
}

int st_app_parse_args(struct st_app_context* ctx, struct mtl_init_params* p, int argc,
                      char** argv) {
  int cmd = -1, optIdx = 0;
  int nb;

  while (1) {
    cmd = getopt_long_only(argc, argv, "hv", st_app_args_options, &optIdx);
    //if(cmd == -2) break;
    //cmd = ST_ARG_CONFIG_FILE;
    if (cmd == -1) break;
    logger->trace("{}, cmd {} {}", __func__, cmd, optarg);

    switch (cmd) {
      case ST_ARG_P_PORT:
        snprintf(p->port[MTL_PORT_P], sizeof(p->port[MTL_PORT_P]), "%s", optarg);
        p->num_ports++;
        break;
      case ST_ARG_R_PORT:
        snprintf(p->port[MTL_PORT_R], sizeof(p->port[MTL_PORT_R]), "%s", optarg);
        p->num_ports++;
        break;
      case ST_ARG_P_SIP:
        inet_pton(AF_INET, optarg, mtl_p_sip_addr(p));
        break;
      case ST_ARG_R_SIP:
        inet_pton(AF_INET, optarg, mtl_r_sip_addr(p));
        break;
      case ST_ARG_HDR_SPLIT:
        ctx->enable_hdr_split = true;
        break;
      case ST_ARG_PACING_WAY:
        if (!strcmp(optarg, "auto"))
          p->pacing = ST21_TX_PACING_WAY_AUTO;
        else if (!strcmp(optarg, "rl"))
          p->pacing = ST21_TX_PACING_WAY_RL;
        else if (!strcmp(optarg, "tsn"))
          p->pacing = ST21_TX_PACING_WAY_TSN;
        else if (!strcmp(optarg, "tsc"))
          p->pacing = ST21_TX_PACING_WAY_TSC;
        else if (!strcmp(optarg, "ptp"))
          p->pacing = ST21_TX_PACING_WAY_PTP;
        else
          logger->error("{}, unknow pacing way {}", __func__, optarg);
        break;
      case ST_ARG_CONFIG_FILE:
        //optarg = "./test_tx_1port_1v_1a_1anc.json";
        app_args_json(ctx, p, optarg);
        ctx->config_file_path = optarg;
        //cmd = -2;
        break;
      case ST_ARG_PTP_UNICAST_ADDR:
        p->flags |= MTL_FLAG_PTP_UNICAST_ADDR;
        break;
      case ST_ARG_CNI_THREAD:
        p->flags |= MTL_FLAG_CNI_THREAD;
        break;
      case ST_ARG_TEST_TIME:
        ctx->test_time_s = atoi(optarg);
        break;
      case ST_ARG_RX_EBU:
        p->flags |= MTL_FLAG_RX_VIDEO_EBU;
        break;
      case ST_ARG_RX_MONO_POOL:
        p->flags |= MTL_FLAG_RX_MONO_POOL;
        break;
      case ST_ARG_TX_MONO_POOL:
        p->flags |= MTL_FLAG_TX_MONO_POOL;
        break;
      case ST_ARG_MONO_POOL:
        p->flags |= MTL_FLAG_RX_MONO_POOL;
        p->flags |= MTL_FLAG_TX_MONO_POOL;
        break;
      case ST_ARG_RX_POOL_DATA_SIZE:
        p->rx_pool_data_size = atoi(optarg);
        break;
      case ST_ARG_RX_SEPARATE_VIDEO_LCORE:
        p->flags |= MTL_FLAG_RX_SEPARATE_VIDEO_LCORE;
        break;
      case ST_ARG_RX_MIX_VIDEO_LCORE:
        p->flags &= ~MTL_FLAG_RX_SEPARATE_VIDEO_LCORE;
        break;
      case ST_ARG_TSC_PACING:
        p->pacing = ST21_TX_PACING_WAY_TSC;
        break;
      case ST_ARG_USER_LCORES:
        app_args_parse_lcores(p, optarg);
        break;
      case ST_ARG_SCH_DATA_QUOTA:
        p->data_quota_mbs_per_sch = atoi(optarg);
        break;
      case ST_ARG_SCH_SESSION_QUOTA: /* unit: 1080p tx */
        nb = atoi(optarg);
        if (nb > 0 && nb < 100) {
          p->data_quota_mbs_per_sch = nb * st20_1080p59_yuv422_10bit_bandwidth_mps();
        }
        break;
      case ST_ARG_NIC_RX_PROMISCUOUS:
        p->flags |= MTL_FLAG_NIC_RX_PROMISCUOUS;
        break;
      case ST_ARG_LIB_PTP:
        p->flags |= MTL_FLAG_PTP_ENABLE;
        p->ptp_get_time_fn = NULL; /* clear the user ptp func */
        break;
      case ST_ARG_PTP_PI:
        p->flags |= MTL_FLAG_PTP_PI;
        break;
      case ST_ARG_PTP_KP:
        p->kp = strtod(optarg, NULL);
        break;
      case ST_ARG_PTP_KI:
        p->ki = strtod(optarg, NULL);
        break;
      // case ST_ARG_PTP_TSC:
      //   p->flags |= MTL_FLAG_PTP_SOURCE_TSC;
      //   break;
      case ST_ARG_LOG_LEVEL:
        if (!strcmp(optarg, "debug"))
          p->log_level = MTL_LOG_LEVEL_DEBUG;
        else if (!strcmp(optarg, "info"))
          p->log_level = MTL_LOG_LEVEL_INFO;
        else if (!strcmp(optarg, "notice"))
          p->log_level = MTL_LOG_LEVEL_NOTICE;
        else if (!strcmp(optarg, "warning"))
          p->log_level = MTL_LOG_LEVEL_WARNING;
        else if (!strcmp(optarg, "error"))
          p->log_level = MTL_LOG_LEVEL_ERROR;
        else
          logger->error("{}, unknow log level {}", __func__, optarg);
        //app_set_log_level(p->log_level);
        break;
      case ST_ARG_NB_TX_DESC:
        p->nb_tx_desc = atoi(optarg);
        break;
      case ST_ARG_NB_RX_DESC:
        p->nb_rx_desc = atoi(optarg);
        break;
      case ST_ARG_DMA_DEV:
        app_args_dma_dev(p, optarg);
        break;
      case ST_ARG_RUNTIME_SESSION:
        ctx->runtime_session = true;
        break;
      case ST_ARG_AF_XDP_ZC_DISABLE:
        p->flags |= MTL_FLAG_AF_XDP_ZC_DISABLE;
        break;
      case ST_ARG_START_QUEUE:
        p->xdp_info[MTL_PORT_P].start_queue = atoi(optarg);
        p->xdp_info[MTL_PORT_R].start_queue = atoi(optarg);
        break;
      case ST_ARG_P_START_QUEUE:
        p->xdp_info[MTL_PORT_P].start_queue = atoi(optarg);
        break;
      case ST_ARG_R_START_QUEUE:
        p->xdp_info[MTL_PORT_R].start_queue = atoi(optarg);
        break;
      case ST_ARG_TASKLET_TIME:
        p->flags |= MTL_FLAG_TASKLET_TIME_MEASURE;
        break;
      case ST_ARG_UTC_OFFSET:
        ctx->utc_offset = atoi(optarg);
        break;
      case ST_ARG_NO_SYSTEM_RX_QUEUES:
        p->flags |= MTL_FLAG_DISABLE_SYSTEM_RX_QUEUES;
        break;
      case ST_ARG_TX_COPY_ONCE:
        ctx->tx_copy_once = true;
        break;
      case ST_ARG_TASKLET_SLEEP:
        p->flags |= MTL_FLAG_TASKLET_SLEEP;
        break;
      case ST_ARG_TASKLET_THREAD:
        p->flags |= MTL_FLAG_TASKLET_THREAD;
        break;
      case ST_ARG_APP_THREAD:
        ctx->app_thread = true;
        break;
      case ST_ARG_RXTX_SIMD_512:
        p->flags |= MTL_FLAG_RXTX_SIMD_512;
        break;
      case ST_ARG_HTTP_SERVER_PORT:
        ctx->http_port = std::atoi(optarg);
        break;
      /* case ST_ARG_TX_SESSIONS_CNT_MAX:
        {
          int cnt = std::atoi(optarg);
          if (cnt > 0) {
            ctx->para.tx_sessions_cnt_max = cnt;
          }
        }
        break;
      case ST_ARG_RX_SESSIONS_CNT_MAX:
        {
          int cnt = std::atoi(optarg);
          if (cnt > 0) {
            ctx->para.rx_sessions_cnt_max = cnt;
          }
        }
        
        break; */
      case ST_ARG_DECKLINK_ID_MAP:
        ctx->decklink_id_map = optarg;
        break;
      /* case ST_ARG_MTL_DETECT_RETRY:
        {
          int retry = std::atoi(optarg);
          mtl_seeder_dev_set_detect_retry(retry);
        }
        break; */
      case '?':
        break;
      default:
        break;
    }
  };

  return 0;
}

void st_app_init_logger_from_args(struct st_app_context* ctx, int argc, char **argv) {
  namespace po = boost::program_options;

  std::string log_level = "info";
  std::string console_level;
  std::string file_level;
  std::string log_path;
  int max_size_mb = 10;
  int max_files = 30;
  po::options_description desc("Command Line Options For Logger");
  desc.add_options()
    ("log_level", po::value(&log_level), "logging level")
    ("console_log_level", po::value(&console_level), "logging level for console")
    ("log_path", po::value(&log_path), "logging output file path")
    ("file_log_level", po::value(&file_level), "logging level for file")
    ("file_log_max_size", po::value(&max_size_mb), "logging output file max size in MB")
    ("file_log_max_files", po::value(&max_files), "logging output file max rotation files")
    ("node_config_file", po::value(&ctx->nmos_node_config_file), "nmos node config file");
  po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
  po::variables_map po_vm;
  po::store(parsed, po_vm);
  po::notify(po_vm);

  if (console_level.empty()) {
    console_level = log_level;
  }
  std::vector<spdlog::sink_ptr> sinks;
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  sinks.push_back(console_sink);
  console_sink->set_level(spdlog::level::from_str(console_level));

  if (!log_path.empty()) {
    if (file_level.empty()) {
      file_level = log_level;
    }
    auto size = 1048576 * max_size_mb ; //MB
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path, size, max_files);
    if (file_sink) {
      sinks.push_back(file_sink);
    }
    file_sink->set_level(spdlog::level::from_str(file_level));
  }
  seeder::core::logger = std::make_shared<spdlog::logger>("SEEDER ST2110", sinks.begin(), sinks.end());
  logger->set_level(spdlog::level::from_str(log_level));
}
