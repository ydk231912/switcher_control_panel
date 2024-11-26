#include "server.h"

namespace seeder{

class Server::Impl{
public:
    void init() {
        switcher.reset(new Switcher(config_path));
        switcher->init();
        control_panel_config = switcher->control_panel_config;

        uart_port1_config.reset(new UARTConfig{control_panel_config->panel_config.uart_port1});
        uart_port2_config.reset(new UARTConfig{control_panel_config->panel_config.uart_port2});

        data_frame.reset(new DataFrame(control_panel_config, switcher));

        main_controller.reset(new MainController(*uart_port1_config,*uart_port2_config, control_panel_config ,data_frame, switcher));

        activate_config_update_notify();

        can_socket.reset(new CanSocket(control_panel_config->panel_config.can_port));
        can_frame_processor.reset(new CanFrameProcessor(*can_socket, switcher));
    }

    void start() {
        can_frame_processor->start();
        set_canframe_handler();

        set_switcher_handler();
        
        main_controller->sendFrames();
    }

    void stop() {
        m_abort = true;
        main_controller->stop();
        can_frame_processor->stop();
    }

    void set_canframe_handler() {
        can_frame_processor->on_transition_press = [&] (const std::string &transition) {
            main_controller->handle_transition_press(transition);
        };
        // 代理键第一行按键功能
        can_frame_processor->on_proxy_press = [&] (const int proxy_idx, const std::string &proxy_type) {
            main_controller->handle_proxy_press(proxy_idx, proxy_type);
            main_controller->sendFrames();
        };
        can_frame_processor->on_shift_press = [&] {
            handle_config_update();
        };
    }


    void set_switcher_handler() {
        switcher->set_panel_status_handler(
            std::bind(&seeder::MainController::handler_status, main_controller,
                  std::placeholders::_1, std::placeholders::_2));
        switcher->set_dsk_status_handler(
            std::bind(&seeder::MainController::handler_dsk_status, main_controller,
                    std::placeholders::_1));
        switcher->set_transition_type_handler(
            std::bind(&seeder::MainController::handler_transition_type, main_controller,
                    std::placeholders::_1));
        switcher->set_config_update_notify_handler(
            std::bind(&seeder::Server::Impl::handle_config_update_notify, this, 
                    std::placeholders::_1));
        switcher->set_panel_status_update_handler(
            std::bind(&seeder::CanFrameProcessor::handle_panel_status_update, can_frame_processor, 
                    std::placeholders::_1));
        switcher->set_proxy_sources_update_handler(
            std::bind(&seeder::MainController::handle_proxy_sources_press, main_controller, 
                    std::placeholders::_1));
        switcher->set_get_key_status_handler(
            std::bind(&seeder::MainController::handle_get_key_status, main_controller, 
                    std::placeholders::_1));
    }
    
    void handle_config_update() {
        activate_config_update_notify();
        main_controller->sendFrames();
    }

    void activate_config_update_notify(){
        switcher->store_screens_config(screen_frame_hex_data, switcher->pgm_commands, switcher->pvw_commands, switcher->proxy_keys_commands);
        main_controller->screen_frame_data = main_controller->data_frame_->construct_frame1(0xA8, 0x01, screen_frame_hex_data);
    }

    void handle_config_update_notify(nlohmann::json param) {
        if (param["type"] == "panel_bus_config_update") {
            handle_config_update();
        }
    }

    void check_can_uart_link_handle(){
        if (!can_socket->checkLinkStatus()) {
            logger->error("CAN link is down, attempting to reconnect...");
            can_socket->reconnect();
        }
        if(!main_controller->uart1_.checklinkstatus() || !main_controller->uart3_.checklinkstatus()) {
            logger->error("CAN link is down, attempting to reconnect...");
            main_controller->uart1_.reconnect();
            main_controller->uart3_.reconnect();
        }
    }

    std::shared_ptr<Switcher> switcher;
    std::shared_ptr<config::Config> control_panel_config;

    std::shared_ptr<CanSocket> can_socket;
    std::shared_ptr<CanFrameProcessor> can_frame_processor;
    std::shared_ptr<UARTConfig> uart_port1_config;
    std::shared_ptr<UARTConfig> uart_port2_config;
    std::shared_ptr<DataFrame> data_frame;
    std::shared_ptr<MainController> main_controller;

    std::string config_path;

    std::vector<std::vector<std::vector<std::string>>> screen_frame_hex_data;

    std::atomic<bool> m_abort = false;
};

Server::Server(std::string config_path) : p_impl(new Impl){
    p_impl->config_path = config_path;
    p_impl->init();
}

Server::~Server(){}

void Server::start(){
    p_impl->start();
}

void Server::stop(){
    p_impl->stop();
}

void Server::join(){
    while(!p_impl->m_abort){
        p_impl->check_can_uart_link_handle();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
} // namespace seeder