#include "delay_http_service.h"

#include <fmt/format.h>

#include "config_util.h"
#include "server/config_service.h"
#include "server/delay_control_service.h"
#include "server/http_service.h"
#include "st2110/st2110_config.h"
#include "st2110/st2110_service.h"


namespace seeder {

class DelayHttpService::Impl {
public:

#define HTTP_ROUTE(method, path, handler) \
    http_service->add_route(HttpMethod::method, path, [this] (const HttpRequest &req, HttpResponse &resp) handler)

    void setup_http() {
        HTTP_ROUTE(Get, "/api/delay/device_config", {
            resp.json_ok(this->get_device_info());
        });

        HTTP_ROUTE(Post, "/api/delay/update_delay_device_config", {
            auto j = req.parse_json_body();
            update_delay_device_config(j);
            resp.json_ok();
        });

        HTTP_ROUTE(Get, "/api/delay/get_fmt", {
            get_legacy_fmts(req, resp);
        });
    }

    nlohmann::json get_device_info() {
        auto device_info = delay_control_service->get_device_info_async().get();
        auto &j_stream = device_info["delay_stream"];
        if (j_stream.contains("main_output")) {
            // output 的返回内容仅保留部分字段，因为其他参数必须和input保持一致
            seeder::config::ST2110OutputConfig output_config = j_stream["main_output"];
            nlohmann::json j_output;
            j_output["device_id"] = output_config.device_id;
            auto &j_video = j_output["video"];
            j_video["ip"] = output_config.video.ip;
            j_video["port"] = output_config.video.port;
            j_video["payload_type"] = output_config.video.payload_type;
            auto &j_audio = j_output["audio"];
            j_audio["ip"] = output_config.audio.ip;
            j_audio["port"] = output_config.audio.port;
            j_audio["payload_type"] = output_config.audio.payload_type;
            j_stream["main_output"] = j_output;
        }
        return device_info;
    }

    void update_delay_device_config(nlohmann::json &param) {
        auto device_info = delay_control_service->get_device_info_async().get();
        auto &j_stream = device_info["delay_stream"];
        seeder::config::ST2110InputConfig old_input_config = j_stream["main_input"];
        seeder::config::ST2110InputConfig new_input_config = param["main_input"];
        new_input_config.enable = true;
        new_input_config.video.enable = true;
        seeder::config::ST2110OutputConfig old_output_config = j_stream["main_output"];
        seeder::config::ST2110OutputConfig new_output_config = param["main_output"];
        // output的部分参数必须和input保持一致
        new_output_config.enable = true;
        new_output_config.video.enable = true;
        new_output_config.video.video_format = new_input_config.video.video_format;
        new_output_config.audio.enable = new_input_config.audio.enable;
        new_output_config.audio.audio_format = new_input_config.audio.audio_format;
        new_output_config.audio.audio_sampling = new_input_config.audio.audio_sampling;
        new_output_config.audio.audio_channel = new_input_config.audio.audio_channel;

        seeder::config::validate_input(new_input_config);
        seeder::config::validate_output(new_output_config);
        nlohmann::json r;
        auto input_compare_result = seeder::config::compare_input(old_input_config, new_input_config);
        bool has_change = false;
        if (input_compare_result.has_any_changed()) {
            r["main_input"] = new_input_config;
            has_change = true;
        }
        auto output_compare_result = seeder::config::compare_output(old_output_config, new_output_config);
        if (output_compare_result.has_any_changed()) {
            r["main_output"] = new_output_config;
            has_change = true;
        }
        if (has_change) {
            delay_control_service->update_device_config_async(r).get();
        }
    }

    template<class K, class V>
    static nlohmann::json make_fmt_item(K &&name, V &&value) {
        return nlohmann::json({
            {"name", std::forward<K>(name)},
            {"value", std::forward<V>(value)}
        });
    }

    void get_legacy_fmts(const HttpRequest &req, HttpResponse &resp) {
        using nlohmann::json_pointer;

        nlohmann::json root;
        auto st2110_config = st2110_config_proxy.get();
        for (int i = 0; i < st2110_config.devices.size(); ++i) {
            const auto &device = st2110_config.devices[i];
            std::string inf_name = fmt::format("WAN{}", i + 1);
            if (!device.sip.empty()) {
                inf_name.append(" (").append(device.sip).append(")");
            }
            root["st2110_input.device_id"].push_back(make_fmt_item(inf_name, device.id));
            root["st2110_output.device_id"].push_back(make_fmt_item(inf_name, device.id));
        }

        struct FmtGroup {
            std::string name;
            std::vector<nlohmann::json> values;
        };
        std::vector<FmtGroup> fmt_groups = {
            {
                "video.video_format",
                {
                    make_fmt_item("2160p50", "2160p5000"),
                    make_fmt_item("1080p50", "1080p5000"),
                }
            },
            {
                "audio.audio_format",
                {
                    make_fmt_item("16", "PCM16"),
                    make_fmt_item("24", "PCM24"),
                }
            },
            {
                "audio.audio_channel",
                {
                    make_fmt_item("ST", "ST"),
                }
            },
            {
                "audio.audio_sampling",
                {
                    make_fmt_item("48kHz", "48kHz"),
                }
            },
        };
        std::vector<std::string> session_types {
            "st2110_input",
            "st2110_output"
        };
        for (auto &group : fmt_groups) {
            for (auto &s : session_types) {
                std::string key = s + "." + group.name;
                root[key] = group.values;
            }
        }

        resp.json_ok(root);
    }

    ConfigCategoryProxy<seeder::config::ST2110Config> st2110_config_proxy;
    std::shared_ptr<HttpService> http_service;
    std::shared_ptr<DelayControlService> delay_control_service;
};

DelayHttpService::DelayHttpService(): p_impl(new Impl) {}

DelayHttpService::~DelayHttpService() {}

void DelayHttpService::setup(ServiceSetupCtx &ctx) {
    auto config_service = ctx.get_service<ConfigService>();
    p_impl->st2110_config_proxy.init_get(*config_service, St2110Service::CONFIG_NAME);
    ctx.get_service(p_impl->http_service);
    ctx.get_service(p_impl->delay_control_service);
    p_impl->setup_http();
}


} // namespace seeder