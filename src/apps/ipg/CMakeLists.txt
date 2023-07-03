file(GLOB_RECURSE SRC_LIST ./*.cpp ./*.c)

add_executable(ipg ${SRC_LIST})

# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/json SRC)
# add_library(calc STATIC ${SRC})

find_package(jsoncpp REQUIRED)

target_link_libraries(
    ipg
    PRIVATE
    core
    decklink
    sdl
    ffmpeg
    
    ffmpeg_deps
    sdl2_deps
    boost_deps
    decklink_deps
    mtl_deps
    jsonc_deps
    spdlog_deps
    jsoncpp_lib_static
)

target_include_directories(
    ipg
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(ipg PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# file(COPY ./ipg_config.xml DESTINATION  ${PROJECT_SOURCE_DIR}/build/bin)
file(COPY ./test_tx_1port_1v_1a_1anc.json DESTINATION  ${CMAKE_BINARY_DIR}/bin)