#ifndef NANOVG_H
#define NANOVG_H

#include <stdint.h>

struct NVGcontext;
typedef struct NVGcontext NVGcontext;

#define NVG_ANTIALIAS 1
#define NVG_STENCIL_STROKES 2

struct NVGcolor {
	union {
		float rgba[4];
		struct { float r,g,b,a; };
	};
};
typedef struct NVGcolor NVGcolor;

inline NVGcolor nvgRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	NVGcolor color;
	color.r = r / 255.0f; color.g = g / 255.0f; color.b = b / 255.0f; color.a = a / 255.0f;
	return color;
}

struct NVGpaint {
	NVGcolor innerColor;
	NVGcolor outerColor;
	float xform[6];
	float extent[2];
	int image;
	float alpha;
};

// Core API
void nvgBeginFrame(NVGcontext* ctx, float windowWidth, float windowHeight, float devicePixelRatio);
void nvgEndFrame(NVGcontext* ctx);

void nvgBeginPath(NVGcontext* ctx);
void nvgRect(NVGcontext* ctx, float x, float y, float w, float h);
void nvgRoundedRect(NVGcontext* ctx, float x, float y, float w, float h, float r);
void nvgCircle(NVGcontext* ctx, float cx, float cy, float r);

void nvgFillColor(NVGcontext* ctx, NVGcolor color);
void nvgFill(NVGcontext* ctx);
void nvgStrokeColor(NVGcontext* ctx, NVGcolor color);
void nvgStrokeWidth(NVGcontext* ctx, float size);
void nvgStroke(NVGcontext* ctx);

void nvgSave(NVGcontext* ctx);
void nvgRestore(NVGcontext* ctx);
void nvgTranslate(NVGcontext* ctx, float x, float y);
void nvgScale(NVGcontext* ctx, float x, float y);
void nvgMoveTo(NVGcontext* ctx, float x, float y);
void nvgBezierTo(NVGcontext* ctx, float c1x, float c1y, float c2x, float c2y, float x, float y);
void nvgClosePath(NVGcontext* ctx);

void nvgTextBounds(NVGcontext* ctx, float x, float y, const char* string, const char* end, float* bounds);

NVGpaint nvgImagePattern(NVGcontext* ctx, float ox, float oy, float ex, float ey, float angle, int image, float alpha);
void nvgFillPaint(NVGcontext* ctx, NVGpaint paint);
int nvgCreateImage(NVGcontext* ctx, const char* filename, int imageFlags);
void nvgDeleteImage(NVGcontext* ctx, int imageId);
int nvgCreateImageMem(NVGcontext* ctx, int imageFlags, unsigned char* data, int ndata);

// Text API
void nvgFontSize(NVGcontext* ctx, float size);
void nvgFontFace(NVGcontext* ctx, const char* font);
void nvgTextAlign(NVGcontext* ctx, int align);
void nvgText(NVGcontext* ctx, float x, float y, const char* string, const char* end);

NVGcontext* nvgCreateGL3(int flags);
void nvgDeleteGL3(NVGcontext* ctx);

enum NVGalign {
	NVG_ALIGN_LEFT = 1<<0, NVG_ALIGN_CENTER = 1<<1, NVG_ALIGN_RIGHT = 1<<2,
	NVG_ALIGN_TOP = 1<<3,  NVG_ALIGN_MIDDLE = 1<<4, NVG_ALIGN_BOTTOM = 1<<5,
};

#endif
