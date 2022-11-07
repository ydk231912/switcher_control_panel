include_directories(./)
include_directories(${PROGRAM_INCLUDE_PATH})

link_directories(${PROJECT_SOURCE_DIR}/build/lib)
# aux_source_directory(./ SRC_LIST)
file(GLOB_RECURSE SRC_LIST ./*.cpp ./*.c ./*.h)

add_executable(ipg ${SRC_LIST})

target_link_libraries(ipg
core
decklink
sdl
ffmpeg
${LINKER_LIBS}
)

# file(COPY ./ipg_config.xml DESTINATION  ${PROJECT_SOURCE_DIR}/build/bin)
file(COPY ./test_tx_1port_1v_1a_1anc.json DESTINATION  ${PROJECT_SOURCE_DIR}/build/bin)