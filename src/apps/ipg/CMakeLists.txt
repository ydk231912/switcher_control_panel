configure_file(app_build_config.h.in app_build_config.h @ONLY)

add_executable(
    ipg

    app_base.cpp
    app_stat.cpp
    app_session.cpp
    app_error_code.cpp
    args.cpp
    fmt.cpp
    http_server.cpp
    main.cpp
    parse_json.cpp
    # player.cpp
    rx_audio.cpp
    rx_video.cpp
    tx_audio.cpp
    tx_video.cpp   
     node_implementation.cpp
    node.cpp
    st_config_utils.cpp
)

# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/json SRC)
# add_library(calc STATIC ${SRC})

target_link_libraries(
    ipg
    PRIVATE
    core
    decklink
    # sdl
    # ffmpeg
    
    ffmpeg_deps
    # sdl2_deps
    boost_deps
    decklink_deps
    mtl_deps
    jsonc_deps
    spdlog_deps
    json_cpp_deps
    httplib

    nmos-cpp::compile-settings
    nmos-cpp::nmos-cpp

    TracyClient
)

target_include_directories(
    ipg
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    "${CMAKE_CURRENT_BINARY_DIR}"
)

set_target_properties(ipg PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# file(COPY ./ipg_config.xml DESTINATION  ${PROJECT_SOURCE_DIR}/build/bin)
# file(COPY ./test_tx_1port_1v_1a_1anc.json DESTINATION  ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)
install(
    TARGETS ipg
    RUNTIME
)