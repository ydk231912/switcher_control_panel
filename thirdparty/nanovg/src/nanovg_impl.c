#include "nanovg_impl.h"

#include <glad/gl.h>

#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"


struct NVGcontext *nanovg_impl_create(const struct SeederNvgConfig *c) {
    int flags = 0;
    if (c->enable_debug) {
        flags |= NVG_DEBUG;
    }
    if (c->enable_antialias) {
        flags |= NVG_ANTIALIAS;
    }
    if (c->enable_stencil_strokes) {
        flags |= NVG_STENCIL_STROKES;
    }
    return nvgCreateGL3(flags);
}

void nanovg_impl_free(struct NVGcontext *vg) {
    nvgDeleteGL3(vg);
}

int seeder_nvg_gl_register_texture(
    struct NVGcontext *vg, 
    GLuint tex_id,
    enum NVGtexture tex_type,
    int width,
    int height,
    int image_flags
) {
    // 参考 glnvg__renderCreateTexture 和 nvglCreateImageFromHandleGL3

    GLNVGcontext* gl = (GLNVGcontext*)nvgInternalParams(vg)->userPtr;
	GLNVGtexture* tex = glnvg__allocTexture(gl);

	if (tex == NULL) return 0;

    tex->tex = tex_id;
    tex->width = width;
	tex->height = height;
	tex->type = tex_type;
	tex->flags = image_flags | NVG_IMAGE_NODELETE;

    return tex->id;
}