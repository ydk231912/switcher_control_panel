
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ffmpeg
find_package(FFmpeg REQUIRED)
# message("FFMPEG_LIBRARIES=${FFMPEG_LIBRARIES}")
# message("FFMPEG_INCLUDE_DIRS=${FFMPEG_INCLUDE_DIRS}")
# message("FFMPEG_DEFINITIONS=${FFMPEG_DEFINITIONS}")

add_library(ffmpeg_deps INTERFACE)
target_link_libraries(ffmpeg_deps INTERFACE ${FFMPEG_LIBRARIES})
target_include_directories(ffmpeg_deps INTERFACE ${FFMPEG_INCLUDE_DIRS})
if(${FFMPEG_DEFINITIONS})
    target_compile_options(
        ffmpeg_deps
        INTERFACE
        ${FFMPEG_DEFINITIONS}
    )
endif()

# SDL2
find_package(SDL2 REQUIRED)
add_library(sdl2_deps INTERFACE)
target_link_libraries(sdl2_deps INTERFACE ${SDL2_LIBRARIES})
target_include_directories(sdl2_deps INTERFACE ${SDL2_INCLUDE_DIRS})

# Boost
find_package(Boost REQUIRED
    COMPONENTS
    filesystem
    thread
    timer
    chrono
    iostreams
    program_options
)
add_library(boost_deps INTERFACE)
target_link_libraries(
    boost_deps 
    INTERFACE 
    Boost::headers 
    Boost::filesystem 
    Boost::thread 
    Boost::timer
    Boost::chrono
    Boost::iostreams
    Boost::program_options
)

# DeckLink
find_package(DeckLink REQUIRED)
add_library(decklink_deps INTERFACE)
target_link_libraries(decklink_deps INTERFACE ${DECKLINK_LIBRARIES})

# Media-Transport-Library
find_package(MTL REQUIRED)
add_library(mtl_deps INTERFACE)
target_link_libraries(mtl_deps INTERFACE MTL)

# json-c
find_package(JsonC REQUIRED)
add_library(jsonc_deps INTERFACE)
target_link_libraries(jsonc_deps INTERFACE JsonC)

# spdlog
find_package(spdlog REQUIRED)
add_library(spdlog_deps INTERFACE)
target_link_libraries(spdlog_deps INTERFACE spdlog::spdlog)

add_subdirectory(libs)
add_subdirectory(apps)

# aux_source_directory(./ SRC_LIST)
# add_executable(app ${SRC_LIST})

# target_link_libraries(app
#     PRIVATE
#     core
#     rtp
#     st2110
#     net
#     decklink
#     sdl

#     ffmpeg_deps
#     sdl2_deps
#     boost_deps
#     decklink_deps
#     mtl_deps
#     jsonc_deps
#     spdlog_deps
# )