
add_library(
    rtp

    src/consumer/rtp_st2110_consumer.cpp
    src/input/rtp_st2110_input.cpp
    src/output/rtp_st2110_output.cpp
    src/producer/rtp_st2110_producer.cpp
    src/packet.cpp
    src/packets_buffer.cpp
    src/util.cpp
)

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
