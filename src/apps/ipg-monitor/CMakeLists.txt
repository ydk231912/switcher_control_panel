find_package(Boost REQUIRED
    COMPONENTS
    filesystem
    program_options
)

add_executable(
    ipg-monitor

    main.cpp
)

target_link_libraries(
    ipg-monitor

    PRIVATE
    httplib
    Boost::headers
    Boost::filesystem
    Boost::program_options
    json_cpp_deps
    spdlog_deps
)

include(GNUInstallDirs)
install(
    TARGETS ipg-monitor
    TYPE RUNTIME
)

configure_file(active-dpdk.service.in ${CMAKE_CURRENT_BINARY_DIR}/active-dpdk.service @ONLY)
configure_file(ipg-monitor.service.in ${CMAKE_CURRENT_BINARY_DIR}/ipg-monitor.service @ONLY)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/active-dpdk.service
    ${CMAKE_CURRENT_BINARY_DIR}/ipg-monitor.service

    # /usr/lib/systemd/system/
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system
)
