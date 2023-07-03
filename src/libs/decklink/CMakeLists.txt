file(GLOB_RECURSE SRC_LIST src/*.cpp)

add_library(decklink ${SRC_LIST})

target_include_directories(
    decklink
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    decklink
    PUBLIC
    core
    ffmpeg_deps
    decklink_deps
)