#pragma once
#include "nanovg.h"

#include <glad/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SeederNvgConfig {
    int enable_debug;
    int enable_antialias;
    int enable_stencil_strokes;
};

struct NVGcontext *nanovg_impl_create(const struct SeederNvgConfig *c);

void nanovg_impl_free(struct NVGcontext *vg);

// 类似于nvgCreateImageRGBA，返回image id，若失败则返回0
// tex_type: enum NVGtexture
// image_flags: enum NVGimageFlags
// 可以用 nvgDeleteImage 释放
int seeder_nvg_gl_register_texture(
    struct NVGcontext *vg, 
    GLuint tex_id,
    enum NVGtexture tex_type,
    int width,
    int height,
    int image_flags
);


#ifdef __cplusplus
} // extern "C"
#endif