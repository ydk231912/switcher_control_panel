cmake_minimum_required(VERSION 3.16)

project(st2110_dvr LANGUAGES C CXX CUDA)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(thirdparty/tracy)

execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE PROJECT_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT PROJECT_GIT_HASH)
    set(PROJECT_GIT_HASH "NO_COMMIT")
endif()

include(GNUInstallDirs)
add_subdirectory(src)
