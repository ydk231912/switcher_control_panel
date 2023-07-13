add_library(
    decklink

    src/consumer/decklink_consumer.cpp
    src/input/decklink_input.cpp
    src/output/decklink_output.cpp
    src/producer/decklink_producer.cpp
    src/util.cpp
)

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