aux_source_directory(./src SRC_LIST)
include_directories(./include)


set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)
file(COPY ./include/ DESTINATION  ${PROJECT_BINARY_DIR}/include/st2110)

add_library(st2110 ${SRC_LIST})
