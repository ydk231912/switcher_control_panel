find_package(nlohmann_json REQUIRED)
add_library(json_deps INTERFACE)
target_link_libraries(
    json_deps
    INTERFACE
    nlohmann_json::nlohmann_json
)

# libhv
# https://github.com/ithewei/libhv
find_package(libhv REQUIRED)

configure_file(app_build_config.h.in app_build_config.h @ONLY)

add_executable(
    st2110_delay

    main.cpp

    st2110/st2110_service.cpp
    st2110/st2110_session.cpp
    st2110/st2110_dummy.cpp

    server/stat_service.cpp
    server/service_manager.cpp
    server/config_service.cpp
    server/app_ctx.cpp
    server/service.cpp
    server/http_service.cpp

    util/os_util.cpp
    util/io.cpp
    util/uri.cpp
    util/fs.cpp
)

target_include_directories(
    st2110_delay
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(
    st2110_delay
    PRIVATE
    
    core
    json_deps
    mtl_deps
    boost_deps
    hv_static
    TracyClient
)