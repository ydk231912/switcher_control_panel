#include "app_stat.h"
#include "app_base.h"
#include "tx_video.h"
#include "tx_audio.h"
#include "rx_video.h"
#include "rx_audio.h"
#include <chrono>
#include <decklink/input/decklink_input.h>
#include <decklink/output/decklink_output.h>
#include <json/json.h>
#include <json/value.h>
#include <unordered_map>

#include <boost/format.hpp>

namespace {

struct session_stat_entry {
    std::vector<st_app_tx_video_session *> tx_video_sessions;
    std::vector<st_json_video_session_t *> tx_video_sessions_json;
    std::vector<st_app_tx_audio_session *> tx_audio_sessions;
    std::vector<st_json_audio_session_t *> tx_audio_sessions_json;
    seeder::decklink::decklink_input *decklink_input = nullptr;
    std::vector<st_app_rx_video_session *> rx_video_sessions;
    std::vector<st_json_video_session_t *> rx_video_sessions_json;
    std::vector<st_app_rx_audio_session *> rx_audio_sessions;
    std::vector<st_json_audio_session_t *> rx_audio_sessions_json;
    seeder::decklink::decklink_output *decklink_output = nullptr;
};

std::string calc_fps(int num, int seconds) {
    if (seconds <= 0) {
        return "NaN";
    }
    return (boost::format("%.1f") % (static_cast<double>(num) / static_cast<double>(seconds))).str();
}

std::unordered_map<std::string, session_stat_entry> collect_sessions(st_app_context *ctx) {
    std::unordered_map<std::string, session_stat_entry> session_map;
    for (auto &s : ctx->tx_video_sessions) {
        session_map[s->tx_source_id].tx_video_sessions.push_back(s.get());
    }
    for (auto &s : ctx->json_ctx->tx_video_sessions) {
        session_map[s.base.id].tx_video_sessions_json.push_back(&s);
    }
    for (auto &s : ctx->tx_audio_sessions) {
        session_map[s->tx_source_id].tx_audio_sessions.push_back(s.get());
    }
    for (auto &s : ctx->json_ctx->tx_audio_sessions) {
        session_map[s.base.id].tx_audio_sessions_json.push_back(&s);
    }
    for (auto &[id, s] : ctx->tx_sources) {
        auto decklink_input = dynamic_cast<seeder::decklink::decklink_input *>(s.get());
        if (decklink_input) {
            session_map[id].decklink_input = decklink_input;
        }
    }
    for (auto &s : ctx->rx_video_sessions) {
        session_map[s->rx_output_id].rx_video_sessions.push_back(s.get());
    }
    for (auto &s : ctx->json_ctx->rx_video_sessions) {
        session_map[s.base.id].rx_video_sessions_json.push_back(&s);
    }
    for (auto &s : ctx->rx_audio_sessions) {
        session_map[s->rx_output_id].rx_audio_sessions.push_back(s.get());
    }
    for (auto &s : ctx->json_ctx->rx_audio_sessions) {
        session_map[s.base.id].rx_audio_sessions_json.push_back(&s);
    }
    for (auto &[id, s] : ctx->rx_output) {
        auto decklink_output = dynamic_cast<seeder::decklink::decklink_output *>(s.get());
        if (decklink_output) {
            session_map[id].decklink_output = decklink_output;
        }
    }
    return session_map;
}

} // namespace

void st_app_update_stat(struct st_app_context *ctx) {
    Json::Value stat;
    auto &tx_sessions = stat["tx_sessions"];
    auto &rx_sessions = stat["rx_sessions"];
    auto session_map = collect_sessions(ctx);

    for (auto &[id, s] : session_map) {
        if (s.decklink_input) {
            Json::Value tx_stat;
            tx_stat["id"] = id;

            auto decklink_stat = s.decklink_input->get_stat();
            tx_stat["decklink_frame_cnt"] = decklink_stat.frame_cnt;
            tx_stat["decklink_fps"] = calc_fps(decklink_stat.frame_cnt, ctx->para.dump_period_s);
            tx_stat["invalid_frame_cnt"] = decklink_stat.invalid_frame_cnt;

            for (auto &tx_s : s.tx_video_sessions) {
                tx_stat["device_id"] = tx_s->source_info->device_id;
                tx_stat["video_next_frame_stat"] = tx_s->next_frame_stat.load();
                tx_stat["video_next_frame_not_ready_stat"] = tx_s->next_frame_not_ready_stat.load();
                int frame_done_cnt = tx_s->frame_done_stat.load();;
                tx_stat["video_frame_done_stat"] = frame_done_cnt;
                tx_stat["tx_fps"] = calc_fps(frame_done_cnt, ctx->para.dump_period_s);
                tx_stat["video_build_frame_stat"] = tx_s->build_frame_stat.load();

                st_app_tx_video_session_reset_stat(tx_s);
            }

            for (auto &tx_s : s.tx_audio_sessions) {
                tx_stat["audio_next_frame_stat"] = tx_s->next_frame_stat.load();
                tx_stat["audio_next_frame_not_ready_stat"] = tx_s->next_frame_not_ready_stat.load();
                tx_stat["audio_frame_done_stat"] = tx_s->frame_done_stat.load();
                tx_stat["audio_build_frame_stat"] = tx_s->build_frame_stat.load();

                st_app_tx_audio_session_reset_stat(tx_s);
            }

            tx_sessions.append(tx_stat);
        }
        if (s.decklink_output) {
            Json::Value rx_stat;
            rx_stat["id"] = id;

            auto decklink_stat = s.decklink_output->get_stat();
            rx_stat["decklink_frame_cnt"] = decklink_stat.frame_cnt;
            rx_stat["decklink_fps"] = calc_fps(decklink_stat.frame_cnt, ctx->para.dump_period_s);


            for (auto &rx_s : s.rx_video_sessions) {
                rx_stat["device_id"] = rx_s->output_info.device_id;
                rx_stat["video_frame_receive_stat"] = rx_s->frame_receive_stat.load();
                rx_stat["video_frame_drop_stat"] = rx_s->frame_drop_stat.load();

                st_app_rx_video_session_reset_stat(rx_s);
            }

            for (auto &rx_s : s.rx_audio_sessions) {
                rx_stat["audio_frame_receive_stat"] = rx_s->frame_receive_stat.load();
                rx_stat["audio_frame_drop_stat"] = rx_s->frame_drop_stat.load();

                st_app_rx_audio_session_reset_stat(rx_s);
            }


            rx_sessions.append(rx_stat);
        }
    }

    Json::Int64 record_time_count = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    stat["record_time"] = record_time_count;
    stat["dump_period_s"] = ctx->para.dump_period_s;

    ctx->stat = std::move(stat);
}

Json::Value st_app_get_status(struct st_app_context *ctx) {
    Json::Value root;
    auto &tx_sessions = root["tx_sessions"];
    auto &rx_sessions = root["rx_sessions"];
    auto session_map = collect_sessions(ctx);

    for (auto &[id, s] : session_map) {
        if (s.decklink_input) {
            Json::Value tx_stat;
            tx_stat["id"] = id;
            if (!s.tx_video_sessions_json.empty()) {
                auto tx_json = s.tx_video_sessions_json[0];
                if (tx_json->base.enable) {
                    tx_stat["status"] = "OK";
                    if (!s.decklink_input->has_signal) {
                        tx_stat["status"] = "NO SIGNAL";
                    }
                } else {
                    tx_stat["status"] = "DISABLED";
                }
            }
            tx_sessions.append(tx_stat);
        }
        if (s.decklink_output) {
            Json::Value rx_stat;
            rx_stat["id"] = id;
            if (!s.rx_video_sessions_json.empty()) {
                auto rx_json = s.rx_video_sessions_json[0];
                if (rx_json->base.enable) {
                    rx_stat["status"] = "OK";
                    if (!s.rx_video_sessions.empty()) {
                        auto rx_s = s.rx_video_sessions[0];
                        if (rx_s->frame_receive_status_cnt == 0) {
                            rx_stat["status"] = "NO DATA";
                        }
                        rx_s->frame_receive_status_cnt = 0;
                    }
                } else {
                    rx_stat["status"] = "DISABLED";
                }
            }

            rx_sessions.append(rx_stat);
        }
    }
    return root;
}