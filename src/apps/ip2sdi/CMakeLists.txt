include_directories(./)
include_directories(${PROGRAM_INCLUDE_PATH})

link_directories(${PROJECT_SOURCE_DIR}/build/lib)
aux_source_directory(./ SRC_LIST)

add_executable(ip2sdi ${SRC_LIST})

target_link_libraries(ip2sdi
core
rtp
st2110
net
util
decklink
sdl
ffmpeg
${LINKER_LIBS}
)

file(COPY ./ip2sdi_config.xml DESTINATION  ${PROJECT_SOURCE_DIR}/build/bin)