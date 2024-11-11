#pragma once

#include "httplib.h"
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "json.hpp"

namespace seeder {
// Logger
extern std::shared_ptr<spdlog::logger> logger;
class HttpClient {
public:
  HttpClient(std::string host);
  ~HttpClient();

  std::string get(std::string path);
  nlohmann::json get_json(const std::string &path);
  std::string post(std::string path, std::string body);
  std::string Put(const std::string &path, const std::string &body);
  std::string Delete(const std::string &path);
  std::string Patch(const std::string &path, const std::string &body);
std::shared_ptr<httplib::Client> cli_;
private:
  std::string host_;
  
};

} // namespace seeder
