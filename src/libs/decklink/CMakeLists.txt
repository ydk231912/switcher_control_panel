aux_source_directory(./src/producer SRC_LIST)
include_directories(./include)


set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)
file(COPY ./include/ DESTINATION  ${PROJECT_BINARY_DIR}/include/decklink)

add_library(decklink ${SRC_LIST})
