#include "app_session.h"
#include "core/util/logger.h"
#include "tx_video.h"
#include "tx_audio.h"
#include <algorithm>
#include <string>
#include <unordered_map>
#include "app_error_code.h"
#include <boost/range/algorithm/remove_if.hpp>

std::error_code st_app_add_session(st_app_context *ctx, st_json_context *new_json_ctx) {
    int ret = 0;
    // tx source init
    if ((ret = st_tx_video_source_init(ctx, new_json_ctx))) return {ret, st_app_error_category()};
    // tx session init
    if ((ret = st_app_tx_video_sessions_add(ctx, new_json_ctx))) return {ret, st_app_error_category()};
    if ((ret = st_app_tx_audio_sessions_add(ctx, new_json_ctx))) return {ret, st_app_error_category()};
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

std::error_code st_app_update_session(st_app_context *ctx, st_json_context *new_json_ctx) {
    for (auto &s : new_json_ctx->tx_sources) {
        st_app_remove_tx_session(ctx, s.id);
    }
    st_app_add_session(ctx, new_json_ctx);
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

    boost::remove_if(ctx->json_ctx->tx_sources, [&tx_source_id] (auto &s) {
        return s.id == tx_source_id;
    });

    auto &json_tx_sessions = ctx->json_ctx->json_root["tx_sessions"];
    for (int i = 0; i < json_tx_sessions.size(); ) {
        if (json_tx_sessions[i]["id"] == tx_source_id) {
            json_tx_sessions.removeIndex(i, nullptr);
        } else {
            ++i;
        }
    }
    
    return {};
}