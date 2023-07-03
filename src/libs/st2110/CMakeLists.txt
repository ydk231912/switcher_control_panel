aux_source_directory(./src SRC_LIST)

add_library(st2110 ${SRC_LIST})

target_include_directories(
    st2110
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)