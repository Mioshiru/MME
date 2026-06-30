#include "main.h"
#include "renderer.h"
#include "gui.h"
#include "map.h"
#include "nanovg.h"
#include "nanovg_gl.h"

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#endif

#ifdef _WIN32
typedef void(APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void(APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, ptrdiff_t size,
                                            const void *data, GLenum usage);
static PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
static PFNGLBUFFERDATAPROC glBufferData = nullptr;
#endif

namespace RME_Rendering {

OpenGLBackend::OpenGLBackend() {
    m_nvgContext = nullptr;
}

OpenGLBackend::~OpenGLBackend() {
    if (m_nvgContext) {
        nvgDeleteGL3(m_nvgContext);
        m_nvgContext = nullptr;
    }
}

void OpenGLBackend::Initialize(void *windowHandle) {
    // Load OpenGL extension pointers on Windows
#ifdef _WIN32
    if (!glBindBuffer) {
        glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
        glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    }
#endif

    // Setup clean OpenGL 4.5 state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLBackend::BeginFrame() {
    // Let MapDrawer::SetupGL handle viewport and projection based on zoom
}

void OpenGLBackend::EndFrame() {
    glFlush();
}

void OpenGLBackend::UpdateChunk(uint32_t floorId, const std::vector<MapVertex> &vertices) {
    if (vertices.empty()) return;
    
    // Upload vertices directly using standard OpenGL VBO calls
    if (glBindBuffer && glBufferData) {
        glBindBuffer(GL_ARRAY_BUFFER, floorId);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MapVertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void OpenGLBackend::UpdateAtlas(uint32_t width, uint32_t height, void *data) {
    if (!data) return;
    
    // Modern OpenGL texture upload
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void OpenGLBackend::ReloadShaders() {
    // GLSL shader program loading (no-op here since shader source is bundled/embedded)
    wxLogStatus("OpenGL GLSL Shaders reloaded.");
}

void OpenGLBackend::Resize(int w, int h) {
    if (w <= 0 || h <= 0) return;
    m_width = w;
    m_height = h;
    
    // Ensure viewport changes dynamically with glViewport
    glViewport(0, 0, m_width, m_height);
}

::NVGcontext *OpenGLBackend::CreateNanoVGContext() {
    if (!m_nvgContext) {
        m_nvgContext = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    }
    return m_nvgContext;
}

} // namespace RME_Rendering
