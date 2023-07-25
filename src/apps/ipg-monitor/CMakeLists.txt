find_package(Boost REQUIRED
    COMPONENTS
    filesystem
    program_options
)
find_package(jsoncpp REQUIRED)

add_executable(
    ipg-monitor

    main.cpp
)

target_link_libraries(
    ipg-monitor

    PRIVATE
    httplib
    Boost::headers
    Boost::filesystem
    Boost::program_options
    JsonCpp::JsonCpp
    spdlog_deps
)