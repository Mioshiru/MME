//////////////////////////////////////////////////////////////////////
// NanoVG Stub Implementation
// Provides no-op implementations for NanoVG API functions
// to satisfy linker requirements when no real NanoVG library is used.
//////////////////////////////////////////////////////////////////////

#include "nanovg.h"
#include <cstdlib>
#include <cstring>

// Internal dummy context
struct NVGcontext {
    int dummy;
};

static NVGcontext s_dummyCtx = {0};

// Core API
void nvgBeginFrame(NVGcontext*, float, float, float) {}
void nvgEndFrame(NVGcontext*) {}

void nvgBeginPath(NVGcontext*) {}
void nvgRect(NVGcontext*, float, float, float, float) {}
void nvgRoundedRect(NVGcontext*, float, float, float, float, float) {}
void nvgCircle(NVGcontext*, float, float, float) {}

void nvgFillColor(NVGcontext*, NVGcolor) {}
void nvgFill(NVGcontext*) {}
void nvgStrokeColor(NVGcontext*, NVGcolor) {}
void nvgStrokeWidth(NVGcontext*, float) {}
void nvgStroke(NVGcontext*) {}

void nvgSave(NVGcontext*) {}
void nvgRestore(NVGcontext*) {}
void nvgTranslate(NVGcontext*, float, float) {}
void nvgScale(NVGcontext*, float, float) {}
void nvgMoveTo(NVGcontext*, float, float) {}
void nvgBezierTo(NVGcontext*, float, float, float, float, float, float) {}
void nvgClosePath(NVGcontext*) {}

void nvgTextBounds(NVGcontext*, float, float, const char*, const char*, float* bounds) {
    if (bounds) {
        memset(bounds, 0, sizeof(float) * 4);
    }
}

NVGpaint nvgImagePattern(NVGcontext*, float, float, float, float, float, int, float) {
    NVGpaint p = {};
    return p;
}

void nvgFillPaint(NVGcontext*, NVGpaint) {}

int nvgCreateImage(NVGcontext*, const char*, int) {
    return 0;
}

void nvgDeleteImage(NVGcontext*, int) {}

int nvgCreateImageMem(NVGcontext*, int, unsigned char*, int) {
    return 0;
}

// Text API
void nvgFontSize(NVGcontext*, float) {}
void nvgFontFace(NVGcontext*, const char*) {}
void nvgTextAlign(NVGcontext*, int) {}
void nvgText(NVGcontext*, float, float, const char*, const char*) {}

// GL3 backend
NVGcontext* nvgCreateGL3(int) {
    return &s_dummyCtx;
}

void nvgDeleteGL3(NVGcontext*) {}
