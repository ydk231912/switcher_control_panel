option(ENABLE_SEEDER_LICENSE "Enable seeder_license" ON)

if(ENABLE_SEEDER_LICENSE)
    FetchContent_Declare(
        seeder_license
        GIT_REPOSITORY git@seeder-git-server:seeder/seeder_license.git
        GIT_TAG 4fe822d3909d533200b2f1072fd7772be126658e
    )
    FetchContent_MakeAvailable(seeder_license)
endif()

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

target_compile_definitions(
    ipg
    PRIVATE
    $<$<BOOL:${ENABLE_SEEDER_LICENSE}>:HAVE_SEEDER_LICENSE>
)

target_link_libraries(
    ipg
    PRIVATE
    core
    decklink
    
    ffmpeg_deps
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
    $<$<BOOL:${ENABLE_SEEDER_LICENSE}>:seeder_license>
)

target_include_directories(
    ipg
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    "${CMAKE_CURRENT_BINARY_DIR}"
)

set_target_properties(ipg PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)
install(
    TARGETS ipg
    RUNTIME
)