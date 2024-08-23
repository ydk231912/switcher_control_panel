add_library(
    core
    
    src/stream/input.cpp
    src/stream/output.cpp
    src/util/buffer.cpp
    src/util/fs.cpp
    src/util/logger.cpp
    src/util/uuid.cpp
    src/util/thread.cpp
    src/video_format.cpp
)
target_include_directories(
    core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    core

    PUBLIC
    spdlog_deps
    DPDK
)
