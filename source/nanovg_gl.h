#ifndef NANOVG_GL_H
#define NANOVG_GL_H

#define NVG_ANTIALIAS 1
#define NVG_STENCIL_STROKES 2
#define NVG_DEBUG 4

#ifdef NANOVG_GL3_IMPLEMENTATION
NVGcontext* nvgCreateGL3(int flags);
void nvgDeleteGL3(NVGcontext* ctx);
#endif

#ifdef NANOVG_GL4_IMPLEMENTATION
NVGcontext* nvgCreateGL4(int flags);
void nvgDeleteGL4(NVGcontext* ctx);
#endif

#endif
