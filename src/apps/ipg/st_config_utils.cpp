#include "st_config_utils.h"

#define SAFE_LOG(level, ...)                                                   \
  if (seeder::core::logger)                                                    \
  (seeder::core::logger)->level(__VA_ARGS__)

namespace fs = boost::filesystem;
namespace seeder {
namespace utils {
void write_string_to_file_directly(const std::string &file_path,
                                   const std::string &content) {
  std::fstream stream(file_path, std::ios_base::out);
  stream << content;
  stream.flush();
}

void write_string_to_file(const std::string &file_path,
                          const std::string &content) {
  if (fs::exists(file_path)) {
    boost::system::error_code ec;
    std::string new_file_path = file_path + ".new";
    write_string_to_file_directly(new_file_path, content);
    std::string old_file_path = file_path + ".old";
    fs::rename(file_path, old_file_path, ec);
    if (ec) {
      SAFE_LOG(warn, "failed to mv {} {}", file_path, old_file_path,
               ec.message());
      return;
    }
    fs::rename(new_file_path, file_path, ec);
    if (ec) {
      SAFE_LOG(warn, "failed to mv {} {} : {}", new_file_path, file_path,
               ec.message());
      return;
    }
    fs::remove(old_file_path, ec);
    if (ec) {
      SAFE_LOG(warn, "failed to rm {} : {}", old_file_path, ec.message());
      return;
    }
  } else {
    write_string_to_file_directly(file_path, content);
  }
}

std::string read_file_to_string(const std::string &file_path) {
  std::string content;
  std::ifstream file_stream(file_path, std::ios_base::binary);
  // file_stream.exceptions(std::ios::badbit | std::ios::failbit);
  file_stream.seekg(0, std::ios_base::end);
  auto size = file_stream.tellg();
  file_stream.seekg(0);
  content.resize(static_cast<size_t>(size));
  file_stream.read(&content[0], static_cast<std::streamsize>(size));
  return content;
}

void save_config_file(st_app_context *app_ctx) {
  try {
    Json::Reader reader;
    Json::Value config_json;
    reader.parse(read_file_to_string(app_ctx->config_file_path), config_json);
    config_json["tx_sessions"] = app_ctx->json_ctx->json_root["tx_sessions"];
    config_json["rx_sessions"] = app_ctx->json_ctx->json_root["rx_sessions"];
    Json::FastWriter writer;
    std::string output = writer.write(app_ctx->json_ctx->json_root);
    write_string_to_file(app_ctx->config_file_path, output);
    SAFE_LOG(info, "save config file {}", app_ctx->config_file_path);
  } catch (std::exception &e) {
    SAFE_LOG(error, "save config file {}", e.what());
  }
}
} // namespace utils
} // namespace seeder
