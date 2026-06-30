#ifndef RME_RENDERER_H
#define RME_RENDERER_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <vector>
#include <algorithm>
#include <map>
#include <string>

// Include standard OpenGL headers
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

struct NVGcontext;

namespace RME_Rendering {

struct MapVertex {
  float x, y;
  float u, v;
  uint8_t r, g, b, a;
  float shader_data; // 0: statisch, 1: Wasser, 2: Animation
};

enum class BackendType { OpenGL };

class RenderBackend {
public:
  virtual ~RenderBackend() = default;
  virtual void Initialize(void *windowHandle) = 0;
  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;
  virtual void UpdateChunk(uint32_t floorId,
                           const std::vector<MapVertex> &vertices) = 0;
  virtual void UpdateAtlas(uint32_t width, uint32_t height, void *data) = 0;
  virtual void ReloadShaders() = 0;
  virtual void Resize(int w, int h) = 0;
  virtual ::NVGcontext *CreateNanoVGContext() = 0;
};

class OpenGLBackend : public RenderBackend {
public:
  OpenGLBackend();
  ~OpenGLBackend() override;

  void Initialize(void *windowHandle) override;
  void BeginFrame() override;
  void EndFrame() override;
  void UpdateChunk(uint32_t floorId, const std::vector<MapVertex> &vertices) override;
  void UpdateAtlas(uint32_t width, uint32_t height, void *data) override;
  void ReloadShaders() override;
  void Resize(int w, int h) override;
  ::NVGcontext *CreateNanoVGContext() override;

private:
  int m_width = 800;
  int m_height = 600;
  float m_projectionMatrix[16];
  ::NVGcontext *m_nvgContext = nullptr;
};

} // namespace RME_Rendering

#endif // RME_RENDERER_H
