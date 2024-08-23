find_package(OpenGL REQUIRED)

add_library(
    glad
    STATIC
    src/gl.c
)

target_include_directories(
    glad
    PUBLIC
    include
)

target_link_libraries(
    glad
    PUBLIC
    OpenGL::GL
)