#include "app_session.h"
#include "app_base.h"
#include "app_error_code.h"
#include "core/util/logger.h"
#include "rx_audio.h"
#include "rx_video.h"
#include "tx_audio.h"
#include "tx_video.h"
#include <algorithm>
#include <json/value.h>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
// #include <boost/range/algorithm/remove_if.hpp>

using seeder::core::logger;
// using boost::remove_if;

template <class C, class F> static void remove_if(C &c, const F &f) {
  for (auto it = c.begin(); it != c.end();) {
    if (f(*it)) {
      it = c.erase(it);
    } else {
      ++it;
    }
  }
}

static std::vector<st_json_session_base *>
collection_tx_session_base(st_json_context *c) {
  std::vector<st_json_session_base *> v;
  for (auto &s : c->tx_video_sessions) {
    v.push_back(&s.base);
  }
  for (auto &s : c->tx_audio_sessions) {
    v.push_back(&s.base);
  }
  return v;
}

static std::vector<st_json_session_base *>
collection_rx_session_base(st_json_context *c) {
  std::vector<st_json_session_base *> v;
  for (auto &s : c->rx_video_sessions) {
    v.push_back(&s.base);
  }
  for (auto &s : c->rx_audio_sessions) {
    v.push_back(&s.base);
  }
  return v;
}

static bool
check_addr_used(const std::vector<st_json_session_base *> &sessions,
                const std::vector<st_json_session_base *> &new_sessions) {
  // {ip:port} => session.id
  std::unordered_map<std::string, std::string> used_addr;

  for (auto &s : sessions) {
    for (int i = 0; i < MTL_PORT_MAX && i < s->num_inf; ++i) {
      auto ip = st_app_ip_addr_to_string(s->ip[i]);
      if (!ip.empty() && ip != "0.0.0.0") {
        auto key = ip + ":" + std::to_string(s->udp_port[i]);
        used_addr[key] = s->id;
      }
    }
  }
  for (auto &s : new_sessions) {
    for (int i = 0; i < MTL_PORT_MAX && i < s->num_inf; ++i) {
      auto ip = st_app_ip_addr_to_string(s->ip[i]);
      if (!ip.empty() && ip != "0.0.0.0") {
        auto key = ip + ":" + std::to_string(s->udp_port[i]);
        auto it = used_addr.find(key);
        if (it != used_addr.end() && it->second != s->id) {
          logger->warn("check_addr_used failed {} id1={} id2={} "
                       "sessions.size={} new_sessions.size={}",
                       it->first, it->second, s->id, sessions.size(),
                       new_sessions.size());
          return true;
        }
        used_addr[key] = s->id;
      }
    }
  }

  return false;
}

static void
check_empty_id(const std::string &prefix,
               const std::vector<st_json_session_base *> &sessions) {
  for (std::size_t i = 0; i < sessions.size(); ++i) {
    if (sessions[i]->id.empty()) {
      logger->warn(
          "check_empty_id(): prefix={} sessions.size={} index={} empty id",
          prefix, sessions.size(), i);
    }
  }
}

static void check_empty_id(const std::string &prefix, st_app_context *ctx) {
  std::vector<st_json_session_base *> tx_session_base =
      collection_tx_session_base(ctx->json_ctx.get());
  check_empty_id(prefix + " tx_session_base", tx_session_base);
  std::vector<st_json_session_base *> rx_session_base =
      collection_rx_session_base(ctx->json_ctx.get());
  check_empty_id(prefix + " rx_session_base", rx_session_base);
}

static bool
is_decklink_device_support_video_format(const Json::Value &device,
                                        const std::string &video_format) {
  if (!device.isObject() || !device["support_video_format"].isArray()) {
    return true;
  }
  auto &supported_video_format = device["support_video_format"];
  for (Json::Value::ArrayIndex i = 0; i < supported_video_format.size(); ++i) {
    auto f = supported_video_format[i].asString();
    if (video_format.find(f) != std::string::npos) {
      return true;
    }
  }
  return false;
}

static std::error_code
check_decklink_video_format(st_app_context *ctx,
                            st_json_context *new_json_ctx) {
  const auto &devices = ctx->json_ctx->json_root["decklink"]["devices"];
  if (!devices.isArray()) {
    return st_app_errc::SUCCESS;
  }
  std::unordered_map<int, const Json::Value *> device_map;
  for (Json::Value::ArrayIndex i = 0; i < devices.size(); ++i) {
    device_map[i + 1] = &devices[i];
  }
  for (auto &tx_source : new_json_ctx->tx_sources) {
    auto it = device_map.find(tx_source.device_id);
    if (it == device_map.end()) {
      continue;
    }
    if (!is_decklink_device_support_video_format(*it->second,
                                                 tx_source.video_format)) {
      return st_app_errc::UNSUPPORTED_VIDEO_FORMAT;
    }
  }
  for (auto &rx_output : new_json_ctx->rx_output) {
    auto it = device_map.find(rx_output.device_id);
    if (it == device_map.end()) {
      continue;
    }
    if (!is_decklink_device_support_video_format(*it->second,
                                                 rx_output.video_format)) {
      return st_app_errc::UNSUPPORTED_VIDEO_FORMAT;
    }
  }
  return st_app_errc::SUCCESS;
}

static std::error_code
st_app_check_device_status(st_app_context *ctx, st_json_context *new_json_ctx) {
  std::unordered_map<int, std::string> used_device_id;
  for (auto &[id, tx_source] : ctx->source_info) {
    used_device_id[tx_source.device_id] = id;
  }
  for (auto &[id, rx_output] : ctx->output_info) {
    used_device_id[rx_output.device_id] = id;
  }
  // 将转SDI的输出口添加进去排查
  for (int i = 0; i < ctx->tx_video_sessions.size(); i++) {
    for (auto &[id, sdi_output] : ctx->tx_video_sessions[i]->sdi_output_info) {
      used_device_id[sdi_output.device_id] = id;
    }
  }

  for (auto &new_tx_source : new_json_ctx->tx_sources) {
    auto it = used_device_id.find(new_tx_source.device_id);
    if (it != used_device_id.end() && it->second != new_tx_source.id) {
      logger->warn("Device ID USED device_id={} id1={} id2={}", it->first,
                   it->second, new_tx_source.id);
      return st_app_errc::DECKLINK_DEVICE_USED;
    }
    used_device_id[new_tx_source.device_id] = new_tx_source.id;
  }
  for (auto &new_rx_output : new_json_ctx->rx_output) {
    auto it = used_device_id.find(new_rx_output.device_id);
    if (it != used_device_id.end() && it->second != new_rx_output.id) {
      logger->warn("Device ID USED device_id={} id1={} id2={}", it->first,
                   it->second, new_rx_output.id);
      return st_app_errc::DECKLINK_DEVICE_USED;
    }
    used_device_id[new_rx_output.device_id] = new_rx_output.id;
  }

  // 将转SDI的输出口进行筛查
  for (auto &new_sdi_output : new_json_ctx->tx_outputsdi) {
    auto it = used_device_id.find(new_sdi_output.device_id);
    if (it != used_device_id.end() && it->second != new_sdi_output.id) {
      logger->warn("Device ID USED device_id={} id1={} id2={}", it->first,
                   it->second, new_sdi_output.id);
      return st_app_errc::DECKLINK_DEVICE_USED;
    }
    used_device_id[new_sdi_output.device_id] = new_sdi_output.id;
  }

  std::vector<st_json_session_base *> tx_session_base =
      collection_tx_session_base(ctx->json_ctx.get());
  check_empty_id("tx_session_base", tx_session_base);
  std::vector<st_json_session_base *> tx_session_new_base =
      collection_tx_session_base(new_json_ctx);
  check_empty_id("tx_session_new_base", tx_session_new_base);
  if (check_addr_used(tx_session_base, tx_session_new_base)) {
    return st_app_errc::ADDRESS_CONFLICT;
  }
  std::vector<st_json_session_base *> rx_session_base =
      collection_rx_session_base(ctx->json_ctx.get());
  check_empty_id("rx_session_base", rx_session_base);
  std::vector<st_json_session_base *> rx_session_new_base =
      collection_rx_session_base(new_json_ctx);
  check_empty_id("rx_session_new_base", rx_session_new_base);
  if (check_addr_used(rx_session_base, rx_session_new_base)) {
    return st_app_errc::ADDRESS_CONFLICT;
  }

  if (auto ec = check_decklink_video_format(ctx, new_json_ctx)) {
    return ec;
  }

  return st_app_errc::SUCCESS;
}

static std::error_code st_app_update_check_id(st_app_context *ctx,
                                              st_json_context *new_json_ctx) {
  std::vector<st_json_session_base *> tx_session_base =
      collection_tx_session_base(ctx->json_ctx.get());
  std::vector<st_json_session_base *> tx_session_new_base =
      collection_tx_session_base(new_json_ctx);
  std::vector<st_json_session_base *> rx_session_base =
      collection_rx_session_base(ctx->json_ctx.get());
  std::vector<st_json_session_base *> rx_session_new_base =
      collection_rx_session_base(new_json_ctx);
  std::unordered_set<std::string> ids;
  for (auto &s : tx_session_base) {
    ids.insert(s->id);
  }
  for (auto &s : rx_session_base) {
    ids.insert(s->id);
  }
  for (auto &s : tx_session_new_base) {
    if (ids.find(s->id) == ids.end()) {
      return st_app_errc::ID_INVALID;
    }
  }
  for (auto &s : rx_session_new_base) {
    if (ids.find(s->id) == ids.end()) {
      return st_app_errc::ID_INVALID;
    }
  }
  return st_app_errc::SUCCESS;
}

static std::error_code make_device_error(int code) {
  return {code, st_app_device_error_category::instance()};
}

static std::error_code
st_app_add_tx_sessions_device(st_app_context *ctx,
                              st_json_context *new_json_ctx) {
  int ret = 0;
  // tx source init  初始化一个新的tx_source
  if ((ret = st_tx_video_source_init(ctx, new_json_ctx)))
    return make_device_error(ret);
  // tx session init   通过实例化的方式将tx_source放入到context中
  if ((ret = st_app_tx_video_sessions_add(ctx, new_json_ctx)))
    return make_device_error(ret);
  if ((ret = st_app_tx_audio_sessions_add(ctx, new_json_ctx)))
    return make_device_error(ret);

  // sdi_output init 实例化output_sdi信息
  if ((ret = st_app_output_sdi_init(ctx, new_json_ctx)))
    return make_device_error(ret);
  // tx source start
  std::unordered_map<std::string, st_app_tx_source *> json_tx_source_map;
  for (auto &s : new_json_ctx->tx_sources) {
    json_tx_source_map[s.id] = &s;
  }
  for (auto &[source_id, input] : ctx->tx_sources) {
    if (json_tx_source_map.find(source_id) != json_tx_source_map.end()) {
      if (!input->is_started()) {
        input->start();
      }
    }
  }
  // output_sdi start 此处将output_sdi的数据启动
  for (int i = 0; i < ctx->tx_video_sessions.size(); i++) {
    for (auto &[source_id, outputsdi] : ctx->tx_video_sessions[i]->sdi_output) {
      if (!outputsdi->is_started()) {
        outputsdi->start();
      }
    }
  }
  return {};
}

static void
st_app_add_tx_sessions_update_json_ctx(st_app_context *ctx,
                                       st_json_context *new_json_ctx) {
  for (auto &s : new_json_ctx->tx_video_sessions) {
    ctx->json_ctx->tx_video_sessions.push_back(s);
  }
  for (auto &s : new_json_ctx->tx_audio_sessions) {
    ctx->json_ctx->tx_audio_sessions.push_back(s);
  }
  for (auto &s : new_json_ctx->tx_sources) {
    ctx->json_ctx->tx_sources.push_back(s);
  }
  for (auto &v : new_json_ctx->json_root["tx_sessions"]) {
    ctx->json_ctx->json_root["tx_sessions"].append(v);
  }
}

static void
st_app_remove_tx_session_update_json_ctx(st_app_context *ctx,
                                         const std::string &tx_source_id) {
  ctx->tx_sources.erase(tx_source_id);
  ctx->source_info.erase(tx_source_id);

  remove_if(ctx->json_ctx->tx_video_sessions,
            [&tx_source_id](auto &s) { return s.base.id == tx_source_id; });
  remove_if(ctx->json_ctx->tx_audio_sessions,
            [&tx_source_id](auto &s) { return s.base.id == tx_source_id; });
  remove_if(ctx->json_ctx->tx_sources,
            [&tx_source_id](auto &s) { return s.id == tx_source_id; });
  remove_if(ctx->json_ctx->tx_outputsdi,
            [&tx_source_id](auto &s) { return s.id == tx_source_id; });

  auto &json_tx_sessions = ctx->json_ctx->json_root["tx_sessions"];
  for (int i = 0; i < json_tx_sessions.size();) {
    if (json_tx_sessions[i]["id"] == tx_source_id) {
      Json::Value got;
      json_tx_sessions.removeIndex(i, &got);
    } else {
      ++i;
    }
  }
}

static void st_app_remove_tx_session_device(st_app_context *ctx,
                                            const std::string &tx_source_id) {
  for (auto iter = ctx->tx_video_sessions.begin();
       iter != ctx->tx_video_sessions.end();) {
    auto &s = *iter;
    if (s->tx_source_id == tx_source_id) {
      st_app_tx_video_session_uinit(s.get());
      iter = ctx->tx_video_sessions.erase(iter);
      seeder::core::logger->info("remove tx video session {}", tx_source_id);
    } else {
      ++iter;
    }
  }

  for (auto iter = ctx->tx_audio_sessions.begin();
       iter != ctx->tx_audio_sessions.end();) {
    auto &s = *iter;
    if (s->tx_source_id == tx_source_id) {
      st_app_tx_audio_session_uinit(s.get());
      iter = ctx->tx_audio_sessions.erase(iter);
      seeder::core::logger->info("remove tx audio session {}", tx_source_id);
    } else {
      ++iter;
    }
  }
}

std::error_code st_app_add_tx_sessions(st_app_context *ctx,
                                       st_json_context *new_json_ctx) {
  std::error_code ec = st_app_check_device_status(ctx, new_json_ctx);
  if (ec)
    return ec;
  ec = st_app_add_tx_sessions_device(ctx, new_json_ctx);
  if (ec) {
    // 如果初始化session失败的话，则删除已初始化的session
    std::vector<std::string> new_ids;
    for (auto &s : new_json_ctx->tx_sources) {
      new_ids.push_back(s.id);
    }
    for (auto &id : new_ids) {
      st_app_remove_tx_session_device(ctx, id);
    }
    return ec;
  }
  st_app_add_tx_sessions_update_json_ctx(ctx, new_json_ctx);

  check_empty_id("st_app_add_tx_sessions", ctx);

  return {};
}

std::error_code st_app_remove_tx_session(st_app_context *ctx,
                                         const std::string &tx_source_id) {
  st_app_remove_tx_session_device(ctx, tx_source_id);

  st_app_remove_tx_session_update_json_ctx(ctx, tx_source_id);

  check_empty_id("st_app_remove_tx_session", ctx);
  return {};
}

std::error_code st_app_update_tx_sessions(st_app_context *ctx,
                                          st_json_context *new_json_ctx) {
  std::error_code ec = st_app_update_check_id(ctx, new_json_ctx);
  if (ec)
    return ec;
  ec = st_app_check_device_status(ctx, new_json_ctx);
  if (ec)
    return ec;
  for (auto &s : new_json_ctx->tx_sources) {
    st_app_remove_tx_session(ctx, s.id);
  }

  ec = st_app_add_tx_sessions_device(ctx, new_json_ctx);
  if (ec) {
    // 如果初始化session失败的话，则删除已初始化的session
    std::vector<std::string> new_ids;
    for (auto &s : new_json_ctx->tx_sources) {
      new_ids.push_back(s.id);
    }
    for (auto &id : new_ids) {
      st_app_remove_tx_session_device(ctx, id);
    }
  }
  st_app_add_tx_sessions_update_json_ctx(ctx, new_json_ctx);
  check_empty_id("st_app_update_tx_sessions", ctx);
  return {};
}

static void st_app_remove_rx_session_device(st_app_context *ctx,
                                            const std::string &rx_output_id) {
  for (auto iter = ctx->rx_video_sessions.begin();
       iter != ctx->rx_video_sessions.end();) {
    auto &s = *iter;
    if (s->rx_output_id == rx_output_id) {
      st_app_rx_video_session_uinit(s.get());
      iter = ctx->rx_video_sessions.erase(iter);
      seeder::core::logger->info("remove rx video session {}", rx_output_id);
    } else {
      ++iter;
    }
  }

  for (auto iter = ctx->rx_audio_sessions.begin();
       iter != ctx->rx_audio_sessions.end();) {
    auto &s = *iter;
    if (s->rx_output_id == rx_output_id) {
      st_app_rx_audio_session_uinit(s.get());
      iter = ctx->rx_audio_sessions.erase(iter);
      seeder::core::logger->info("remove rx audio session {}", rx_output_id);
    } else {
      ++iter;
    }
  }

  ctx->rx_output.erase(rx_output_id);
  ctx->output_info.erase(rx_output_id);
}

static std::error_code
st_app_add_rx_sessions_device(st_app_context *ctx,
                              st_json_context *new_json_ctx) {
  int ret = 0;
  // tx source init
  if ((ret = st_app_rx_output_init(ctx, new_json_ctx)))
    return make_device_error(ret);
  // tx session init
  if ((ret = st_app_rx_video_sessions_add(ctx, new_json_ctx)))
    return make_device_error(ret);
  if ((ret = st_app_rx_audio_sessions_add(ctx, new_json_ctx)))
    return make_device_error(ret);

  // rx_output start
  std::unordered_map<std::string, st_app_rx_output *> json_rx_output_map;
  for (auto &s : new_json_ctx->rx_output) {
    json_rx_output_map[s.id] = &s;
  }
  for (auto &[source_id, output] : ctx->rx_output) {
    if (json_rx_output_map.find(source_id) != json_rx_output_map.end()) {
      if (!output->is_started()) {
        output->start();
      }
    }
  }

  return {};
}

static void
st_app_add_rx_sessions_update_json_ctx(st_app_context *ctx,
                                       st_json_context *new_json_ctx) {
  for (auto &s : new_json_ctx->rx_video_sessions) {
    ctx->json_ctx->rx_video_sessions.push_back(s);
  }
  for (auto &s : new_json_ctx->rx_audio_sessions) {
    ctx->json_ctx->rx_audio_sessions.push_back(s);
  }
  for (auto &s : new_json_ctx->rx_output) {
    ctx->json_ctx->rx_output.push_back(s);
  }
  for (auto &v : new_json_ctx->json_root["rx_sessions"]) {
    ctx->json_ctx->json_root["rx_sessions"].append(v);
  }
}

std::error_code st_app_add_rx_sessions(st_app_context *ctx,
                                       st_json_context *new_json_ctx) {
  std::error_code ec = st_app_check_device_status(ctx, new_json_ctx);
  if (ec)
    return ec;
  ec = st_app_add_rx_sessions_device(ctx, new_json_ctx);
  if (ec) {
    // 如果初始化session失败的话，则删除已初始化的session
    std::vector<std::string> new_ids;
    for (auto &s : new_json_ctx->rx_output) {
      new_ids.push_back(s.id);
    }
    for (auto &id : new_ids) {
      st_app_remove_rx_session_device(ctx, id);
    }
    return ec;
  }

  st_app_add_rx_sessions_update_json_ctx(ctx, new_json_ctx);

  check_empty_id("st_app_add_rx_sessions", ctx);
  return {};
}

static void
st_app_remove_rx_session_update_json_ctx(st_app_context *ctx,
                                         const std::string &rx_output_id) {
  remove_if(ctx->json_ctx->rx_video_sessions,
            [&rx_output_id](auto &s) { return s.base.id == rx_output_id; });
  remove_if(ctx->json_ctx->rx_audio_sessions,
            [&rx_output_id](auto &s) { return s.base.id == rx_output_id; });
  remove_if(ctx->json_ctx->rx_output,
            [&rx_output_id](auto &s) { return s.id == rx_output_id; });

  auto &json_rx_sessions = ctx->json_ctx->json_root["rx_sessions"];
  for (int i = 0; i < json_rx_sessions.size();) {
    if (json_rx_sessions[i]["id"] == rx_output_id) {
      Json::Value got;
      json_rx_sessions.removeIndex(i, &got);
    } else {
      ++i;
    }
  }
}

std::error_code st_app_remove_rx_session(st_app_context *ctx,
                                         const std::string &rx_output_id) {
  st_app_remove_rx_session_device(ctx, rx_output_id);
  st_app_remove_rx_session_update_json_ctx(ctx, rx_output_id);

  check_empty_id("st_app_remove_rx_session", ctx);
  return {};
}

std::error_code st_app_update_rx_sessions(st_app_context *ctx,
                                          st_json_context *new_json_ctx) {
  std::error_code ec = st_app_update_check_id(ctx, new_json_ctx);
  if (ec)
    return ec;
  ec = st_app_check_device_status(ctx, new_json_ctx);
  if (ec)
    return ec;
  for (auto &s : new_json_ctx->rx_output) {
    st_app_remove_rx_session(ctx, s.id);
  }

  ec = st_app_add_rx_sessions_device(ctx, new_json_ctx);
  if (ec) {
    // 如果初始化session失败的话，则删除已初始化的session
    std::vector<std::string> new_ids;
    for (auto &s : new_json_ctx->rx_output) {
      new_ids.push_back(s.id);
    }
    for (auto &id : new_ids) {
      st_app_remove_rx_session_device(ctx, id);
    }
  }

  st_app_add_rx_sessions_update_json_ctx(ctx, new_json_ctx);
  check_empty_id("st_app_update_rx_sessions", ctx);
  return {};
}