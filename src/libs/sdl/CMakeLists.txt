file(GLOB_RECURSE SRC_LIST src/*.cpp)

add_library(sdl ${SRC_LIST})

target_include_directories(
    sdl
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    sdl
    PUBLIC
    core
    ffmpeg_deps
    sdl2_deps
)