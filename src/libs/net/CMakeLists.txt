aux_source_directory(./src SRC_LIST)
include_directories(./include)
include_directories(${PROGRAM_INCLUDE_PATH})
link_directories(${PROJECT_SOURCE_DIR}/build/lib)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)
file(COPY ./include/ DESTINATION  ${PROJECT_BINARY_DIR}/include/net)

add_library(net ${SRC_LIST})

target_link_libraries(net 
    util
)

