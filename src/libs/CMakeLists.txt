add_library(httplib INTERFACE)
target_include_directories(
    httplib
    
    INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/httplib
)
add_subdirectory(core)
add_subdirectory(st2110)
add_subdirectory(net)
add_subdirectory(rtp)
add_subdirectory(decklink)
add_subdirectory(sdl)
add_subdirectory(ffmpeg)