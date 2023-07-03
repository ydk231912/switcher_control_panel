file(GLOB_RECURSE SRC_LIST src/*.cpp)

add_library(ffmpeg ${SRC_LIST})

target_include_directories(
    ffmpeg
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    ffmpeg
    PUBLIC
    core
    ffmpeg_deps
    sdl2_deps
)