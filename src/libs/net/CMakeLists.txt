aux_source_directory(./src SRC_LIST)

add_library(net ${SRC_LIST})

target_include_directories(
    net
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    net
    PUBLIC 
    core
)

