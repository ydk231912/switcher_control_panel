
# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system filesystem program_options log)
find_package(Threads REQUIRED)
find_package(WebsocketPP REQUIRED)
# 添加 spdlog 库
find_package(spdlog REQUIRED)

include_directories(../lib
                    commumication/)


add_executable(
    switcher_keyboard
    commumication/can_receive.cpp
    commumication/uart_send.cpp
    main.cpp
    commumication/http_client.cpp
    commumication/websocket_client.cpp
    commumication/config.h
    commumication/ascll_convert_16_demo.cpp
    switcher.cpp
    #uart_can_all.cpp
)

target_link_libraries(switcher_keyboard PRIVATE spdlog::spdlog Boost::system Boost::filesystem Threads::Threads Boost::program_options Boost::log)
