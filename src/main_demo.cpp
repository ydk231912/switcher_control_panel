#include <memory>
#include <thread>
#include "server.h"

using json = nlohmann::json;

namespace seeder{
auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("/opt/switcher-keyboard/logs/spdlog.log", 1048576 * 20, 5);
std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("multi_sink_logger", spdlog::sinks_init_list{console_sink, file_sink});
void init_logger(){
  try
  {
    logger->set_pattern("[%n][%Y-%m-%d %H:%M:%S.%e] [%l] [%t]  %v");
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::debug);
  }
  catch (const spdlog::spdlog_ex& ex)
  {
    std::cout << "Log initialization failed: " << ex.what() << std::endl;
  }
}
} // namespace seeder

namespace{

void run(int argc, char** argv){
    seeder::init_logger();
    seeder::logger->info("程序开始运行");
    // 读取配置文件
    namespace po = boost::program_options;
    std::string config_path;
    po::options_description desc("Seeder Switcher Keybaord");
    desc.add_options()("config_file", po::value(&config_path)->required(),
                        "main config file");
    po::parsed_options parsed =
        po::command_line_parser(argc, argv).options(desc).run();
    po::variables_map po_vm;
    po::store(parsed, po_vm);
    /*调用 notify 方法，通知 variables_map 对象中的所有参数值已被设置。这样，config_path 就会包含从命令行传入的配置文件路径*/
    po::notify(po_vm);
    
    seeder::Server server(config_path);
    server.start();
    server.join();
}
} // namespace


int main (int argc, char** argv){
    run(argc, argv);
    return 0;
}