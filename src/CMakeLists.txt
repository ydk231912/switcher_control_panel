
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)
set(PROGRAM_LIB_PATH ${PROJECT_SOURCE_DIR}/build/lib)
set(PROGRAM_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/build/include)


# ffmpeg
set(FFMPEG_INCLUDE_PATH /usr/include)
set(FFMPEG_LIB_PATH /usr/lib/x86_64-linux-gnu)
list(APPEND LINKER_LIBS 
    ${FFMPEG_LIB_PATH}/libavformat.so
    ${FFMPEG_LIB_PATH}/libavdevice.so
    ${FFMPEG_LIB_PATH}/libavcodec.so
    ${FFMPEG_LIB_PATH}/libavfilter.so
    ${FFMPEG_LIB_PATH}/libpostproc.so
    ${FFMPEG_LIB_PATH}/libavutil.so
    ${FFMPEG_LIB_PATH}/libswscale.so
    ${FFMPEG_LIB_PATH}/libswresample.so
    )
include_directories(SYSTEM ${FFMPEG_INCLUDE_PATH})

# Boost
find_package(Boost REQUIRED COMPONENTS filesystem thread timer chrono)
if (NOT Boost_FOUND)
    message("Boost lib can not be found")
endif()
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LINKER_LIBS ${Boost_LIBRARIES})

# SDL
find_package(SDL2 REQUIRED)
if (NOT SDL2_FOUND)
    message("SDL2 lib can not be found")
endif()
message("MESSAGE SDL include::${SDL2_INCLUDE_DIRS}")
include_directories(${SDL2_INCLUDE_DIRS})
list(APPEND LINKER_LIBS ${SDL2_LIBRARIES})

# DeckLink
list(APPEND LINKER_LIBS 
/usr/lib/libDeckLinkAPI.so
/usr/lib/libDeckLinkPreviewAPI.so)


add_subdirectory(libs)

include_directories(${PROGRAM_INCLUDE_PATH})
link_directories(${PROGRAM_LIB_PATH})

include_directories(./)

aux_source_directory(./ SRC_LIST)
add_executable(app ${SRC_LIST})

message("message::${LINKER_LIBS}")


target_link_libraries(app 
rtp
st2110
net
util
decklink
${LINKER_LIBS} 
)