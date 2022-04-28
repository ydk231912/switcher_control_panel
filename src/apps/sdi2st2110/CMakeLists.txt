
include_directories(./)
include_directories(${PROGRAM_INCLUDE_PATH})

link_directories(${PROJECT_SOURCE_DIR}/build/lib)
aux_source_directory(./ SRC_LIST)

add_executable(sdi2st2110 ${SRC_LIST})

target_link_libraries(sdi2st2110
rtp
st2110
net
util
decklink
sdl
${LINKER_LIBS}
)
