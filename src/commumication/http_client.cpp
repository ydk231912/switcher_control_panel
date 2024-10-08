#include "http_client.h"
#include <memory>
#include <string>

namespace seeder {

HttpClient::HttpClient(std::string host) : host_(host) {
  cli_ = std::make_shared<httplib::Client>(host);
   logger->info("HttpClient host : {}",host);
};
HttpClient::~HttpClient(){};

std::string HttpClient::get(std::string path) {
  // 发送一个 HTTP GET 请求到服务器，通常用于请求获取资源或数据
  // 返回一个 std::shared_ptr<httplib::Response>，包含 HTTP 响应的详细信息，如状态码、响应头和响应体
  auto res = cli_->Get(path);
  if (res && res->status == 200) {
    return res->body;
  }
  return "";
}

nlohmann::json HttpClient::get_json(const std::string &path) {
  auto r = this->get(path);
  nlohmann::json j;
  try {
    j = nlohmann::json::parse(r);
  } catch (std::exception &e) {
    logger->warn("get_json {} json::parse error {}", path, e.what());
  }
  logger->info("transfer port");
  return j;
}

std::string HttpClient::post(std::string path, std::string body) {
  // 发送一个 HTTP POST 请求到服务器，通常用于提交数据到服务器（例如表单数据）
  // 返回一个 std::shared_ptr<httplib::Response>，包含 HTTP 响应的详细信息
  auto res = cli_->Post(path, body, "application/json");
  if (res && res->status == 200) {
    return res->body;
  }else{
    auto s = to_string(res.error());
    logger->error("res path: {} error: {}",path,s);
  }
  return "";
}

std::string HttpClient::Put(const std::string &path, const std::string &body){
  // 发送 HTTP PUT 请求，用于更新资源或数据
  auto res = cli_->Put(path, body, "application/json");
  if (res && res->status == 200) {
    return res->body;
  }else{
    auto s = to_string(res.error());

    logger->error("res path: {} error: {}",path,s);
  }
  return "";
}

std::string HttpClient::Delete(const std::string &path){
  // 发送 HTTP DELETE 请求，用于删除资源
  auto res = cli_->Delete(path);
  if (res && res->status == 200) {
    return res->body;
  }else{
    auto s = to_string(res.error());

    logger->error("res path: {} error: {}",path,s);
  }
  return "";
}

std::string HttpClient::Patch(const std::string &path, const std::string &body){
  // 发送 HTTP PATCH 请求，用于部分更新资源
  auto res = cli_->Patch(path, body, "application/json");
  if (res && res->status == 200) {
    return res->body;
  }else{
    auto s = to_string(res.error());

    logger->error("res path: {} error: {}",path,s);
  }
  return "";
}
} // namespace seeder