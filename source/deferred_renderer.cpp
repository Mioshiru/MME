#include "deferred_renderer.h"
#include <bgfx/platform.h>

// Define a struct matching our Instancing Data layout
struct LightInstanceData {
    float posAndRadius[4]; // x, y, radius, padding
    float colorAndIntensity[4]; // r, g, b, intensity
};

DeferredRenderer::DeferredRenderer() {
    m_legacyColorTexture = BGFX_INVALID_HANDLE;
    m_gbufferFBO = BGFX_INVALID_HANDLE;
    m_lightAccumulationFBO = BGFX_INVALID_HANDLE;
}

DeferredRenderer::~DeferredRenderer() {
    shutdown();
}

void DeferredRenderer::init(void* windowHandle, uint16_t width, uint16_t height) {
    bgfx::Init initObj;
    initObj.type = bgfx::RendererType::OpenGL; // Match legacy OpenGL context
    initObj.platformData.nwh = windowHandle;
    initObj.resolution.width = width;
    initObj.resolution.height = height;
    initObj.resolution.reset = BGFX_RESET_VSYNC;
    bgfx::init(initObj);

    // Setup uniform samplers
    s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
    s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
    s_texLightAccum = bgfx::createUniform("s_texLightAccum", bgfx::UniformType::Sampler);

    createQuad();
    // Shaders would normally be loaded from compiled SPIR-V / DXBC binaries.
    // createShaders(); 
}

void DeferredRenderer::resize(uint16_t width, uint16_t height) {
    bgfx::reset(width, height, BGFX_RESET_VSYNC);

    // Recreate G-Buffer
    if (bgfx::isValid(m_gbufferFBO)) {
        bgfx::destroy(m_gbufferFBO);
    }
    m_gbufferNormals = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
    m_gbufferFBO = bgfx::createFrameBuffer(1, &m_gbufferNormals, true);

    // Recreate Accumulation Buffer
    if (bgfx::isValid(m_lightAccumulationFBO)) {
        bgfx::destroy(m_lightAccumulationFBO);
    }
    m_lightAccumulationTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA16F, BGFX_TEXTURE_RT);
    m_lightAccumulationFBO = bgfx::createFrameBuffer(1, &m_lightAccumulationTexture, true);
}

void DeferredRenderer::setLegacyColorTexture(uint32_t glTextureId) {
    if (bgfx::isValid(m_legacyColorTexture)) {
        bgfx::destroy(m_legacyColorTexture);
    }
    m_legacyColorTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8);
    bgfx::overrideInternal(m_legacyColorTexture, (uintptr_t)glTextureId);
}

void DeferredRenderer::createQuad() {
    bgfx::VertexLayout layout;
    layout.begin()
          .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
          .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
          .end();

    float vertices[] = {
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
    };
    uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };

    m_quadVBO = bgfx::createVertexBuffer(bgfx::makeRef(vertices, sizeof(vertices)), layout);
    m_quadIBO = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));
}

void DeferredRenderer::render(const LightingSystem& lightingSystem) {
    if (!bgfx::isValid(m_legacyColorTexture) || !bgfx::isValid(m_lightPassProgram)) {
        return; // Not ready
    }

    // 1. Clear accumulation buffer to ambient light color
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f, 0);
    bgfx::setViewFrameBuffer(0, m_lightAccumulationFBO);
    bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);
    bgfx::touch(0);

    // 2. Setup Instancing for Lights
    auto lights = lightingSystem.getVisibleLights(0, 0, 10000, 10000); // TODO: pass actual camera frustum
    if (!lights.empty()) {
        const uint16_t instanceStride = sizeof(LightInstanceData);
        uint32_t numInstances = bgfx::getAvailInstanceDataBuffer(static_cast<uint32_t>(lights.size()), instanceStride);
        
        bgfx::InstanceDataBuffer idb;
        bgfx::allocInstanceDataBuffer(&idb, numInstances, instanceStride);

        LightInstanceData* data = (LightInstanceData*)idb.data;
        for (uint32_t i = 0; i < numInstances; ++i) {
            data[i].posAndRadius[0] = lights[i].x;
            data[i].posAndRadius[1] = lights[i].y;
            data[i].posAndRadius[2] = lights[i].radius;
            data[i].posAndRadius[3] = 0.0f; // Padding

            // Extract RGBA
            data[i].colorAndIntensity[0] = ((lights[i].color_rgba >> 24) & 0xFF) / 255.0f;
            data[i].colorAndIntensity[1] = ((lights[i].color_rgba >> 16) & 0xFF) / 255.0f;
            data[i].colorAndIntensity[2] = ((lights[i].color_rgba >> 8) & 0xFF) / 255.0f;
            data[i].colorAndIntensity[3] = lights[i].intensity;
        }

        bgfx::setVertexBuffer(0, m_quadVBO);
        bgfx::setIndexBuffer(m_quadIBO);
        bgfx::setInstanceDataBuffer(&idb);

        // Additive blending for lights
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ADD);
        
        // Bind Normal Map G-Buffer
        bgfx::setTexture(0, s_texNormal, m_gbufferNormals);
        bgfx::submit(0, m_lightPassProgram);
    }

    // 3. Final Composition Pass (Blend Albedo with Light Accumulation)
    bgfx::setViewClear(1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(1, 0, 0, bgfx::BackbufferRatio::Equal);
    
    bgfx::setVertexBuffer(0, m_quadVBO);
    bgfx::setIndexBuffer(m_quadIBO);
    bgfx::setTexture(0, s_texColor, m_legacyColorTexture);
    bgfx::setTexture(1, s_texLightAccum, m_lightAccumulationTexture);
    
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::submit(1, m_compositionProgram);

    // Tell bgfx to swap buffers
    bgfx::frame();
}

void DeferredRenderer::shutdown() {
    // Cleanup resources
    bgfx::shutdown();
}
