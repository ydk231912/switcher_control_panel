#include "can_receive.h"
#include "switcher.h"
#include "uart_send.h"
#include <boost/program_options.hpp>

namespace seeder{

// 创建控制台 sink
auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
// 创建文件 sink
auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("/opt/switcher-keyboard/logs/spdlog.log", 1048576 * 20, 5);
// 创建多 sink 日志记录器
std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("multi_sink_logger", spdlog::sinks_init_list{console_sink, file_sink});

// std::shared_ptr<spdlog::logger> logger = spdlog::basic_logger_mt("spdlog", "logs/spdlog.log");
void init_logger(){
  try
  {
    // 设置日志格式. 参数含义: [日志标识符] [日期] [日志级别] [线程号] [数据]
    logger->set_pattern("[%n][%Y-%m-%d %H:%M:%S.%e] [%l] [%t]  %v");
    logger->set_level(spdlog::level::debug);
    // spdlog::flush_every(std::chrono::seconds(5)); // 定期刷新日志缓冲区
    // 刷新
    logger->flush_on(spdlog::level::debug);
  }
  catch (const spdlog::spdlog_ex& ex)
  {
    std::cout << "Log initialization failed: " << ex.what() << std::endl;
  }
}
};//namespace seeder



// 主函数
int main(int argc, char **argv) {
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
  std::shared_ptr<seeder::Switcher> switcher;
  try {
    switcher = std::make_shared<seeder::Switcher>(config_path);
    switcher->init();

    seeder::UARTConfig config1{"/dev/ttyS1"}; // 串口1配置
    seeder::UARTConfig config3{"/dev/ttyS3"}; // 串口3配置
    
    seeder::MainController controller(config1, config3); // 创建主控制器对象
    controller.switcher = switcher;
    controller.data_frame_.switcher = switcher;
    
    // auto screen_frame_hex_data = controller.data_frame_.readHexData("/opt/switcher-keyboard/datas/data.txt");
    // controller.screen_frame_data = controller.data_frame_.construct_frame1(0xA8, 0x01, screen_frame_hex_data);

    auto screen_frame_hex_data = switcher->store_screens_config();
    controller.screen_frame_data = controller.data_frame_.construct_frame1(0xA8, 0x01, screen_frame_hex_data);

    // controller.sendFrames();

    switcher->set_panel_status_handler(
        std::bind(&seeder::MainController::handler_status, &controller,
                  std::placeholders::_1, std::placeholders::_2));
    switcher->set_dsk_status_handler(
        std::bind(&seeder::MainController::handler_dsk_status, &controller,
                  std::placeholders::_1));
    switcher->set_transition_type_handler(
        std::bind(&seeder::MainController::handler_transition_type, &controller,
                  std::placeholders::_1));

    // switcher->set_next_transition_handler(
    //     std::bind(&seeder::MainController::handler_nextkey, &controller,
    //               std::placeholders::_1));
    // switcher->set_proxy_type_handler(
    //     std::bind(&seeder::MainController::handler_prevtrans, &controller,
    //               std::placeholders::_1));
    // switcher->set_proxy_source_handler(
    //     std::bind(&seeder::MainController::handler_prevtrans, &controller,
    //               std::placeholders::_1));
    // switcher->set_mode_status_handler(
    //     std::bind(&seeder::MainController::handler_mode, &controller,
    //               std::placeholders::_1));
    // switcher->set_shift_status_handler(
    //     std::bind(&seeder::MainController::handler_shift, &controller,
    //               std::placeholders::_1));
    
    seeder::CanSocket canSocket(CAN_INTERFACE);
    seeder::CanFrameProcessor frameProcessor(canSocket,
                                     switcher);
    frameProcessor.on_key_press = [&] (const std::string &k) {
      controller.handle_key_press(k);
    };
    frameProcessor.init();
    frameProcessor.start();

    controller.sendFrames();

    while (true) 
    {
      usleep(1000000); // 等待 1 秒
    }
  
    controller.stop();
    frameProcessor.stop();

    seeder::logger->info("程序已停止");

  } catch (const std::runtime_error &e) {
    seeder::logger->error("运行时错误: {}", e.what());
    return 1;
  }

  return 0;
} //main