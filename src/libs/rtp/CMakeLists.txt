
file(GLOB_RECURSE SRC_LIST src/*.cpp)

add_library(rtp ${SRC_LIST})

target_include_directories(
    rtp
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    rtp
    PUBLIC
    core
    st2110
    net
)
