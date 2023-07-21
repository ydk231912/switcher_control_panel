#include "app_session.h"
#include "app_base.h"
#include "core/util/logger.h"
#include "tx_video.h"
#include "tx_audio.h"
#include "rx_video.h"
#include "rx_audio.h"
#include <algorithm>
#include <json/value.h>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include "app_error_code.h"
#include <boost/range/algorithm/remove_if.hpp>

using seeder::core::logger;

static std::vector<st_json_session_base *> collection_tx_session_base(st_json_context *c) {
    std::vector<st_json_session_base *> v;
    for (auto &s : c->tx_video_sessions) {
        v.push_back(&s.base);
    }
    for (auto &s : c->tx_audio_sessions) {
        v.push_back(&s.base);
    }
    for (auto &s : c->tx_anc_sessions) {
        v.push_back(&s.base);
    }
    for (auto &s : c->tx_st22p_sessions) {
        v.push_back(&s.base);
    }
    for (auto &s : c->tx_st20p_sessions) {
        v.push_back(&s.base);
    }
    return v;
}

static std::vector<st_json_session_base *> collection_rx_session_base(st_json_context *c) {
    std::vector<st_json_session_base *> v;
    for (auto &s : c->rx_video_sessions) {
        v.push_back(&s.base);
    }
    for (auto &s : c->rx_audio_sessions) {
        v.push_back(&s.base);
    }
    for (auto &s : c->rx_anc_sessions) {
        v.push_back(&s.base);
    }
    for (auto &s : c->rx_st22p_sessions) {
        v.push_back(&s.base);
    }
    for (auto &s : c->rx_st20p_sessions) {
        v.push_back(&s.base);
    }
    return v;
}

static bool check_addr_used(const std::vector<st_json_session_base *> &sessions, const std::vector<st_json_session_base *> &new_sessions) {
    // {ip:port} => session.id
    std::unordered_map<std::string, std::string> used_addr;
    for (int i = 0; i < MTL_PORT_MAX; ++i) {
        for (auto &s : sessions) {
            auto ip = st_app_ip_addr_to_string(s->ip[i]);
            if (!ip.empty() && ip != "0.0.0.0") {
                auto key = ip + ":" + std::to_string(s->udp_port);
                used_addr[key] = s->id;
            }
        }
    }
    for (int i = 0; i < MTL_PORT_MAX; ++i) {
        for (auto &s : new_sessions) {
            auto ip = st_app_ip_addr_to_string(s->ip[i]);
            if (!ip.empty() && ip != "0.0.0.0") {
                auto key = ip + ":" + std::to_string(s->udp_port);
                auto it = used_addr.find(key);
                if (it != used_addr.end() && it->second != s->id) {
                    logger->warn("check_addr_used failed {}", it->first);
                    return true;
                }
                used_addr[key] = s->id;
            }
        }
    }
    
    return false;
}


static std::error_code st_app_check_device_status(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::unordered_map<int, std::string> used_device_id;
    for (auto &[id, tx_source] : ctx->source_info) {
        used_device_id[tx_source.device_id] = id;
    }
    for (auto &[id, rx_output] : ctx->output_info) {
        used_device_id[rx_output.device_id] = id;
    }
    for (auto &new_tx_source : new_json_ctx->tx_sources) {
        auto it = used_device_id.find(new_tx_source.device_id);
        if (it != used_device_id.end() && it->second != new_tx_source.id) {
            return st_app_errc::DECKLINK_DEVICE_USED;
        }
        used_device_id[new_tx_source.device_id] = new_tx_source.id;
    }
    for (auto &new_rx_output : new_json_ctx->rx_output) {
        auto it = used_device_id.find(new_rx_output.device_id);
        if (it != used_device_id.end() && it->second != new_rx_output.id) {
            return st_app_errc::DECKLINK_DEVICE_USED;
        }
        used_device_id[new_rx_output.device_id] = new_rx_output.id;
    }

    std::vector<st_json_session_base *> tx_session_base = collection_tx_session_base(ctx->json_ctx.get());
    std::vector<st_json_session_base *> tx_session_new_base = collection_tx_session_base(new_json_ctx);
    if (check_addr_used(tx_session_base, tx_session_new_base)) {
        return st_app_errc::ADDRESS_CONFLICT;
    }
    std::vector<st_json_session_base *> rx_session_base = collection_rx_session_base(ctx->json_ctx.get());
    std::vector<st_json_session_base *> rx_session_new_base = collection_rx_session_base(new_json_ctx);
    if (check_addr_used(rx_session_base, rx_session_new_base)) {
        return st_app_errc::ADDRESS_CONFLICT;
    }
    
    return st_app_errc::SUCCESS;
}

static std::error_code st_app_update_check_id(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::vector<st_json_session_base *> tx_session_base = collection_tx_session_base(ctx->json_ctx.get());
    std::vector<st_json_session_base *> tx_session_new_base = collection_tx_session_base(new_json_ctx);
    std::vector<st_json_session_base *> rx_session_base = collection_rx_session_base(ctx->json_ctx.get());
    std::vector<st_json_session_base *> rx_session_new_base = collection_rx_session_base(new_json_ctx);
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

static std::error_code st_app_add_tx_sessions_device(st_app_context *ctx, st_json_context *new_json_ctx) {
    int ret = 0;
    // tx source init
    if ((ret = st_tx_video_source_init(ctx, new_json_ctx))) return make_device_error(ret);
    // tx session init
    if ((ret = st_app_tx_video_sessions_add(ctx, new_json_ctx))) return make_device_error(ret);
    if ((ret = st_app_tx_audio_sessions_add(ctx, new_json_ctx))) return make_device_error(ret);

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

    return {};
}

static void st_app_add_tx_sessions_udpate_json_ctx(st_app_context *ctx, st_json_context *new_json_ctx) {
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

static void st_app_remove_tx_session_update_json_ctx(st_app_context *ctx, const std::string &tx_source_id) {
    ctx->tx_sources.erase(tx_source_id);
    ctx->source_info.erase(tx_source_id);

    boost::remove_if(ctx->json_ctx->tx_video_sessions, [&tx_source_id] (auto &s) {
        return s.base.id == tx_source_id;
    });
    boost::remove_if(ctx->json_ctx->tx_audio_sessions, [&tx_source_id] (auto &s) {
        return s.base.id == tx_source_id;
    });
    boost::remove_if(ctx->json_ctx->tx_sources, [&tx_source_id] (auto &s) {
        return s.id == tx_source_id;
    });

    auto &json_tx_sessions = ctx->json_ctx->json_root["tx_sessions"];
    for (int i = 0; i < json_tx_sessions.size(); ) {
        if (json_tx_sessions[i]["id"] == tx_source_id) {
            Json::Value got;
            json_tx_sessions.removeIndex(i, &got);
        } else {
            ++i;
        }
    }
}

static void st_app_remove_tx_session_device(st_app_context *ctx, const std::string &tx_source_id) {
    for (auto iter = ctx->tx_video_sessions.begin(); iter != ctx->tx_video_sessions.end();) {
        auto &s = *iter;
        if (s->tx_source_id == tx_source_id) {
            st_app_tx_video_session_uinit(s.get());
            iter = ctx->tx_video_sessions.erase(iter);
            seeder::core::logger->info("remove tx video session {}", tx_source_id);
        } else {
            ++iter;
        }
    }

    for (auto iter = ctx->tx_audio_sessions.begin(); iter != ctx->tx_audio_sessions.end();) {
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

std::error_code st_app_add_tx_sessions(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::error_code ec = st_app_check_device_status(ctx, new_json_ctx);
    if (ec) return ec;
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
    st_app_add_tx_sessions_udpate_json_ctx(ctx, new_json_ctx);
    
    return {};
}

std::error_code st_app_remove_tx_session(st_app_context *ctx, const std::string &tx_source_id) {
    st_app_remove_tx_session_device(ctx, tx_source_id);

    st_app_remove_tx_session_update_json_ctx(ctx, tx_source_id);
    
    return {};
}

std::error_code st_app_update_tx_sessions(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::error_code ec = st_app_update_check_id(ctx, new_json_ctx);
    if (ec) return ec;
    ec = st_app_check_device_status(ctx, new_json_ctx);
    if (ec) return ec;
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
    st_app_add_tx_sessions_udpate_json_ctx(ctx, new_json_ctx);
    return {};
}


static void st_app_remove_rx_session_device(st_app_context *ctx, const std::string &rx_output_id) {
    for (auto iter = ctx->rx_video_sessions.begin(); iter != ctx->rx_video_sessions.end();) {
        auto &s = *iter;
        if (s->rx_output_id == rx_output_id) {
            st_app_rx_video_session_uinit(s.get());
            iter = ctx->rx_video_sessions.erase(iter);
            seeder::core::logger->info("remove rx video session {}", rx_output_id);
        } else {
            ++iter;
        }
    }

    for (auto iter = ctx->rx_audio_sessions.begin(); iter != ctx->rx_audio_sessions.end();) {
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

static std::error_code st_app_add_rx_sessions_device(st_app_context *ctx, st_json_context *new_json_ctx) {
    int ret = 0;
    // tx source init
    if ((ret = st_app_rx_output_init(ctx, new_json_ctx))) return make_device_error(ret);
    // tx session init
    if ((ret = st_app_rx_video_sessions_add(ctx, new_json_ctx))) return make_device_error(ret);
    if ((ret = st_app_rx_audio_sessions_add(ctx, new_json_ctx))) return make_device_error(ret);

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

static void st_app_add_rx_sessions_update_json_ctx(st_app_context *ctx, st_json_context *new_json_ctx) {
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


std::error_code st_app_add_rx_sessions(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::error_code ec = st_app_check_device_status(ctx, new_json_ctx);
    if (ec) return ec;
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
    
    return {};
}

static void st_app_remove_rx_session_update_json_ctx(st_app_context *ctx, const std::string &rx_output_id) {
    boost::remove_if(ctx->json_ctx->rx_video_sessions, [&rx_output_id] (auto &s) {
        return s.base.id == rx_output_id;
    });
    boost::remove_if(ctx->json_ctx->rx_audio_sessions, [&rx_output_id] (auto &s) {
        return s.base.id == rx_output_id;
    });
    boost::remove_if(ctx->json_ctx->rx_output, [&rx_output_id] (auto &s) {
        return s.id == rx_output_id;
    });

    auto &json_rx_sessions = ctx->json_ctx->json_root["rx_sessions"];
    for (int i = 0; i < json_rx_sessions.size(); ) {
        if (json_rx_sessions[i]["id"] == rx_output_id) {
            Json::Value got;
            json_rx_sessions.removeIndex(i, &got);
        } else {
            ++i;
        }
    }
}

std::error_code st_app_remove_rx_session(st_app_context *ctx, const std::string &rx_output_id) {
    st_app_remove_rx_session_device(ctx, rx_output_id);
    st_app_remove_rx_session_update_json_ctx(ctx, rx_output_id);
    
    return {};
}


std::error_code st_app_update_rx_sessions(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::error_code ec = st_app_update_check_id(ctx, new_json_ctx);
    if (ec) return ec;
    ec = st_app_check_device_status(ctx, new_json_ctx);
    if (ec) return ec;
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
    return {};
}