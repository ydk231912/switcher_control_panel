
include_directories(./)
include_directories(${PROGRAM_INCLUDE_PATH})

link_directories(${PROJECT_SOURCE_DIR}/build/lib)
aux_source_directory(./ SRC_LIST)

add_executable(decklink_test ${SRC_LIST})

target_link_libraries(decklink_test
core
decklink
sdl
ffmpeg
)

#file(COPY ./sdi2ip_config.xml DESTINATION  ${PROJECT_SOURCE_DIR}/build/bin)
