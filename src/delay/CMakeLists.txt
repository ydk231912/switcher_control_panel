find_package(nlohmann_json REQUIRED)
add_library(json_deps INTERFACE)
target_link_libraries(
    json_deps
    INTERFACE
    nlohmann_json::nlohmann_json
)

# ffmpeg
find_package(FFmpeg REQUIRED)

add_library(ffmpeg_deps INTERFACE)
target_link_directories(ffmpeg_deps INTERFACE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg_deps INTERFACE ${FFMPEG_LIBRARIES})
target_include_directories(ffmpeg_deps INTERFACE ${FFMPEG_INCLUDE_DIRS})
if(${FFMPEG_DEFINITIONS})
    target_compile_options(
        ffmpeg_deps
        INTERFACE
        ${FFMPEG_DEFINITIONS}
    )
endif()

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
    server/slice_manager.cpp
    server/slice_service.cpp
    server/config_util.cpp
    server/delay_control_service.cpp
    server/delay_http_service.cpp

    media/ffmpeg_util.cpp

    util/os_util.cpp
    util/io.cpp
    util/uri.cpp
    util/fs.cpp
    util/program_var.cpp
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
    ffmpeg_deps
    hv_static
    TracyClient
)