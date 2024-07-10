add_library(httplib INTERFACE)
target_include_directories(
    httplib
    
    INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/httplib
)
# 如果不用这个编译选项的话，会因为打开的fd过多导致http接口异常
target_compile_definitions(
    httplib
    INTERFACE
    CPPHTTPLIB_USE_POLL
)

add_subdirectory(core)
# add_subdirectory(st2110)
# add_subdirectory(net)
# add_subdirectory(rtp)
add_subdirectory(decklink)
# add_subdirectory(sdl)
# add_subdirectory(ffmpeg)