#ifndef RME_DEFERRED_RENDERER_H
#define RME_DEFERRED_RENDERER_H

#include <bgfx/bgfx.h>
#include <vector>
#include "lighting_system.h"

class DeferredRenderer {
public:
    DeferredRenderer();
    ~DeferredRenderer();

    // Initializes bgfx using the native window handle from wxWidgets
    void init(void* windowHandle, uint16_t width, uint16_t height);
    
    // Resizes the G-Buffer and Accumulation buffer
    void resize(uint16_t width, uint16_t height);

    // Pass the Legacy OpenGL FBO texture ID so bgfx can read it
    void setLegacyColorTexture(uint32_t glTextureId);

    // Renders the lighting pass using Instancing
    void render(const LightingSystem& lightingSystem);

    // Shuts down bgfx
    void shutdown();

private:
    bgfx::TextureHandle m_legacyColorTexture;
    
    // G-Buffer
    bgfx::TextureHandle m_gbufferNormals;
    bgfx::FrameBufferHandle m_gbufferFBO;

    // Light Accumulation
    bgfx::TextureHandle m_lightAccumulationTexture;
    bgfx::FrameBufferHandle m_lightAccumulationFBO;

    // Instancing Data
    bgfx::VertexBufferHandle m_quadVBO;
    bgfx::IndexBufferHandle m_quadIBO;

    bgfx::ProgramHandle m_lightPassProgram;
    bgfx::ProgramHandle m_compositionProgram;
    
    bgfx::UniformHandle s_texColor;
    bgfx::UniformHandle s_texNormal;
    bgfx::UniformHandle s_texLightAccum;

    void createShaders();
    void createQuad();
};

#endif // RME_DEFERRED_RENDERER_H
