file(GLOB_RECURSE SRC_LIST src/*.cpp include/*.h)
include_directories(./include)
include_directories(${PROGRAM_INCLUDE_PATH})
link_directories(${PROJECT_SOURCE_DIR}/build/lib)

# FFMPEG
set(FFMPEG_INCLUDE_PATH /usr/include)
set(FFMPEG_LIB_PATH /usr/lib/x86_64-linux-gnu)
include_directories(SYSTEM ${FFMPEG_INCLUDE_PATH})
list(APPEND FFMPEG_LIBS 
    ${FFMPEG_LIB_PATH}/libavformat.so
    ${FFMPEG_LIB_PATH}/libavdevice.so 
    ${FFMPEG_LIB_PATH}/libavcodec.so
    ${FFMPEG_LIB_PATH}/libavfilter.so
    ${FFMPEG_LIB_PATH}/libpostproc.so
    ${FFMPEG_LIB_PATH}/libavutil.so
    ${FFMPEG_LIB_PATH}/libswscale.so
    ${FFMPEG_LIB_PATH}/libswresample.so
    )

# DeckLink 
list(APPEND DECKLINK_LIBS 
/usr/lib/libDeckLinkAPI.so
/usr/lib/libDeckLinkPreviewAPI.so)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)
file(COPY ./include/ DESTINATION  ${PROJECT_BINARY_DIR}/include/decklink)

add_library(decklink ${SRC_LIST})

target_link_libraries(decklink
core
util
${FFMPEG_LIBS} 
${DECKLINK_LIBS}
)