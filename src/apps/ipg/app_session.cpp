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
#include <unordered_map>
#include "app_error_code.h"
#include <boost/range/algorithm/remove_if.hpp>


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
    return st_app_errc::SUCCESS;
}


std::error_code st_app_add_tx_sessions(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::error_code ec = st_app_check_device_status(ctx, new_json_ctx);
    if (ec) return ec;
    int ret = 0;
    // tx source init
    if ((ret = st_tx_video_source_init(ctx, new_json_ctx))) return {ret, st_app_device_error_category::instance()};
    // tx session init
    if ((ret = st_app_tx_video_sessions_add(ctx, new_json_ctx))) return {ret, st_app_device_error_category::instance()};
    if ((ret = st_app_tx_audio_sessions_add(ctx, new_json_ctx))) return {ret, st_app_device_error_category::instance()};
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
    
    return {};
}



std::error_code st_app_update_tx_sessions(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::error_code ec = st_app_check_device_status(ctx, new_json_ctx);
    if (ec) return ec;
    for (auto &s : new_json_ctx->tx_sources) {
        st_app_remove_tx_session(ctx, s.id);
    }
    st_app_add_tx_sessions(ctx, new_json_ctx);
    return {};
}

std::error_code st_app_remove_tx_session(st_app_context *ctx, const std::string &tx_source_id) {
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
            seeder::core::logger->info("remove tx video session {}", tx_source_id);
        } else {
            ++iter;
        }
    }

    ctx->tx_sources.erase(tx_source_id);
    ctx->source_info.erase(tx_source_id);

    boost::remove_if(ctx->json_ctx->tx_video_sessions, [&tx_source_id] (auto &s) {
        return s.tx_source_id == tx_source_id;
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
    
    return {};
}

std::error_code st_app_add_rx_sessions(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::error_code ec = st_app_check_device_status(ctx, new_json_ctx);
    if (ec) return ec;
    int ret = 0;
    // tx source init
    if ((ret = st_app_rx_output_init(ctx, new_json_ctx))) return {ret, st_app_device_error_category::instance()};
    // tx session init
    if ((ret = st_app_rx_video_sessions_add(ctx, new_json_ctx))) return {ret, st_app_device_error_category::instance()};
    if ((ret = st_app_rx_audio_sessions_add(ctx, new_json_ctx))) return {ret, st_app_device_error_category::instance()};
    // tx source start
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
    
    return {};
}

std::error_code st_app_update_rx_sessions(st_app_context *ctx, st_json_context *new_json_ctx) {
    std::error_code ec = st_app_check_device_status(ctx, new_json_ctx);
    if (ec) return ec;
    for (auto &s : new_json_ctx->rx_output) {
        st_app_remove_rx_session(ctx, s.id);
    }
    st_app_add_rx_sessions(ctx, new_json_ctx);
    return {};
}

std::error_code st_app_remove_rx_session(st_app_context *ctx, const std::string &rx_output_id) {
    for (auto iter = ctx->rx_video_sessions.begin(); iter != ctx->rx_video_sessions.end();) {
        auto &s = *iter;
        if (s->rx_output_id == rx_output_id) {
            st_app_rx_video_session_uinit(s.get());
            iter = ctx->rx_video_sessions.erase(iter);
            seeder::core::logger->info("remove tx video session {}", rx_output_id);
        } else {
            ++iter;
        }
    }

    for (auto iter = ctx->rx_audio_sessions.begin(); iter != ctx->rx_audio_sessions.end();) {
        auto &s = *iter;
        if (s->rx_output_id == rx_output_id) {
            st_app_rx_audio_session_uinit(s.get());
            iter = ctx->rx_audio_sessions.erase(iter);
            seeder::core::logger->info("remove tx video session {}", rx_output_id);
        } else {
            ++iter;
        }
    }

    ctx->rx_output.erase(rx_output_id);
    ctx->output_info.erase(rx_output_id);

    boost::remove_if(ctx->json_ctx->rx_video_sessions, [&rx_output_id] (auto &s) {
        return s.rx_output_id == rx_output_id;
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
    
    return {};
}