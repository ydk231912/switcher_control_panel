add_library(
    core
    
    src/executor/executor.cpp
    src/frame/frame.cpp
    src/stream/input.cpp
    src/stream/output.cpp
    src/util/buffer.cpp
    src/util/color_conversion.cpp
    src/util/fs.cpp
    src/util/lodepng.cpp
    src/util/logger.cpp
    src/util/png_writer.cpp
    src/util/uuid.cpp
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
    ffmpeg_deps
)