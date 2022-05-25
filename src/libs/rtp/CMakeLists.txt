
file(GLOB_RECURSE SRC_LIST src/*.cpp include/*.h)

include_directories(./include)
include_directories(${PROGRAM_INCLUDE_PATH})
link_directories(${PROJECT_SOURCE_DIR}/build/lib)


set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)
file(COPY ./include/ DESTINATION  ${PROJECT_BINARY_DIR}/include/rtp )

add_library(rtp ${SRC_LIST})

target_link_libraries(rtp
    core
    util
    st2110
)
