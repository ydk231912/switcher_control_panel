add_library(
    stb
    STATIC
    src/stb_image_impl.c
)

target_include_directories(
    stb
    PUBLIC
    include
)