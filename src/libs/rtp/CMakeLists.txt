
aux_source_directory(./src SRC_LIST)
include_directories(./include)
include_directories(${PROGRAM_INCLUDE_PATH})
# include_directories(${PROGRAM_INCLUDE_PATH}/st2110/d10)
message("rtp include path: ${PROGRAM_INCLUDE_PATH}/st2110")
link_directories(${PROJECT_SOURCE_DIR}/build/lib)

# target_link_libraries(st2110)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)
file(COPY ./include/ DESTINATION  ${PROJECT_BINARY_DIR}/include/rtp )

add_library(rtp ${SRC_LIST})
