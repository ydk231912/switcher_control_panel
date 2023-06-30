
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)
set(PROGRAM_LIB_PATH ${PROJECT_SOURCE_DIR}/build/lib)
set(PROGRAM_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/build/include)

# ffmpeg
set(FFMPEG_INCLUDE_PATH /usr/include)
set(FFMPEG_LIB_PATH /usr/lib/x86_64-linux-gnu)
set(X86_LIB_PATH /usr/lib/x86_64-linux-gnu)
include_directories(SYSTEM ${FFMPEG_INCLUDE_PATH})

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


# Boost
# find_package(Boost REQUIRED COMPONENTS filesystem thread timer chrono iostreams)
# if (NOT Boost_FOUND)
#     message("Boost lib can not be found")
# endif()
# #message("boost inlcude ${Boost_INCLUDE_DIRS} ${Boost_LIBRARIES}")

# include_directories(${Boost_INCLUDE_DIRS})
# list(APPEND LINKER_LIBS ${Boost_LIBRARIES})
set(BOOST_INLCUDE_PATH /ops/boost/include)
set(BOOST_LIB_PATH /ops/boost/lib)
#include_directories(${BOOST_INLCUDE_PATH})
list(APPEND LINKER_LIBS 
    ${X86_LIB_PATH}/libboost_filesystem.so
    ${X86_LIB_PATH}/libboost_thread.so 
    ${X86_LIB_PATH}/libboost_timer.so
    ${X86_LIB_PATH}/libboost_chrono.so
    ${X86_LIB_PATH}/libboost_iostreams.so
    )

# SDL
find_package(SDL2 REQUIRED)
if (NOT SDL2_FOUND)
    message("SDL2 lib can not be found")
endif()
include_directories(${SDL2_INCLUDE_DIRS})
list(APPEND LINKER_LIBS ${SDL2_LIBRARIES})

# DeckLink
list(APPEND LINKER_LIBS 
/usr/lib/libDeckLinkAPI.so
/usr/lib/libDeckLinkPreviewAPI.so)

# DPDK Media Tranport Library
list(APPEND LINKER_LIBS 
/usr/local/lib/x86_64-linux-gnu/libmtl.so)
include_directories(/usr/local/include)

# json
list(APPEND LINKER_LIBS 
/usr/lib/x86_64-linux-gnu/libjson-c.so)
include_directories(/usr/include/json-c)

# SPDLog
# list(APPEND LINKER_LIBS ${SYSTEM_LIB_PATH}/libspdlog.so)

add_subdirectory(libs)
add_subdirectory(apps)

include_directories(/usr/include)
include_directories(${PROGRAM_INCLUDE_PATH})
link_directories(${PROGRAM_LIB_PATH})

include_directories(./)

aux_source_directory(./ SRC_LIST)
add_executable(app ${SRC_LIST})

target_link_libraries(app
core
rtp
st2110
net
decklink
sdl
${LINKER_LIBS} 
)