add_library(
    nanovg
    STATIC

    src/nanovg.c
    src/nanovg_impl.c
)

target_include_directories(
    nanovg
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(
    nanovg
    PUBLIC
    stb
    glad
)