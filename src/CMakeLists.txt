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

# Media-Transport-Library
find_package(MTL REQUIRED)
find_package(DPDK REQUIRED)
add_library(mtl_deps INTERFACE)
target_link_libraries(mtl_deps INTERFACE MTL DPDK)

# spdlog
find_package(spdlog REQUIRED)
add_library(spdlog_deps INTERFACE)
target_link_libraries(spdlog_deps INTERFACE spdlog::spdlog)

add_subdirectory(libs)

add_subdirectory(delay)