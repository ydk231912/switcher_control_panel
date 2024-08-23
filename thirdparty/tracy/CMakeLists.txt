find_package(Threads REQUIRED)

set(TRACY_PUBLIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/public)

add_library(
    TracyClient 
    STATIC

    ${TRACY_PUBLIC_DIR}/TracyClient.cpp
)

target_compile_features(TracyClient PUBLIC cxx_std_11)

target_include_directories(
    TracyClient
    PUBLIC
    ${TRACY_PUBLIC_DIR}
)

target_link_libraries(
    TracyClient
    PUBLIC
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

macro(set_option option help value)
    option(${option} ${help} ${value})
    if(${option})
        message(STATUS "${option}: ON")
        target_compile_definitions(TracyClient PUBLIC ${option})
    else()
        message(STATUS "${option}: OFF")
    endif()
endmacro()

set_option(TRACY_ENABLE "Enable profiling" OFF)
set_option(TRACY_ON_DEMAND "On-demand profiling" OFF)