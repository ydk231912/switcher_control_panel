file(GLOB_RECURSE SRC_LIST src/*.cpp)

add_library(core ${SRC_LIST})
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