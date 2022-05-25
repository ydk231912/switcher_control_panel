
include_directories(./)
include_directories(${PROGRAM_INCLUDE_PATH})

link_directories(${PROJECT_SOURCE_DIR}/build/lib)
aux_source_directory(./ SRC_LIST)

add_executable(sdi2ip ${SRC_LIST})

target_link_libraries(sdi2ip
core
rtp
st2110
net
util
decklink
sdl
${LINKER_LIBS}
)

file(COPY ./sdi2ip_config.xml DESTINATION  ${PROJECT_SOURCE_DIR}/build/bin)
