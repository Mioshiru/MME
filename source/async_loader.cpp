#include "async_loader.h"
#include "gui.h"
#include "main.h"
#include "render_backend.h"
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace RME::Core {

AsyncLoader::AsyncLoader() : m_running(false) {}

AsyncLoader::~AsyncLoader() {
  stop();
  closeMappedFiles();
}

void AsyncLoader::start() {
  if (m_running)
    return;
  m_running = true;
  m_workerThread = std::thread(&AsyncLoader::workerLoop, this);
}

void AsyncLoader::stop() {
  m_running = false;
  m_condition.notify_all();
  if (m_workerThread.joinable()) {
    m_workerThread.join();
  }
}

void AsyncLoader::queueSpriteLoad(uint32_t spriteId,
                                  const std::string &filePath) {
  std::lock_guard<std::mutex> lock(m_queueMutex);
  m_pendingTasks.push({spriteId, filePath});
  m_condition.notify_one();
}

AsyncLoader::MappedFile *AsyncLoader::getMappedFile(const std::string &path) {
  if (path.empty())
    return nullptr;
  auto it = m_mappedFiles.find(path);
  if (it != m_mappedFiles.end())
    return &it->second;

  MappedFile mapped;
#ifdef _WIN32
  mapped.hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (mapped.hFile != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER size;
    GetFileSizeEx(mapped.hFile, &size);
    mapped.size = size.QuadPart;
    mapped.hMapping =
        CreateFileMappingA(mapped.hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapped.hMapping != NULL) {
      mapped.data = MapViewOfFile(mapped.hMapping, FILE_MAP_READ, 0, 0, 0);
    }
  }
#endif
  m_mappedFiles[path] = mapped;
  return &m_mappedFiles[path];
}

void AsyncLoader::closeMappedFiles() {
#ifdef _WIN32
  for (auto &pair : m_mappedFiles) {
    if (pair.second.data)
      UnmapViewOfFile(pair.second.data);
    if (pair.second.hMapping)
      CloseHandle(pair.second.hMapping);
    if (pair.second.hFile && pair.second.hFile != INVALID_HANDLE_VALUE)
      CloseHandle(pair.second.hFile);
  }
#endif
  m_mappedFiles.clear();
}

void AsyncLoader::workerLoop() {
  while (m_running) {
    SpriteLoadTask task;
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_condition.wait(
          lock, [this]() { return !m_pendingTasks.empty() || !m_running; });
      if (!m_running)
        break;

      task = m_pendingTasks.front();
      m_pendingTasks.pop();
    }

    MappedFile *mapped = getMappedFile(task.filePath);
    if (!mapped || !mapped->data) {
      // Fallback, wenn Datei fehlt
      std::vector<uint8_t> dummyData(32 * 32 * 4, 255);
      std::lock_guard<std::mutex> lock(m_resultMutex);
      m_loadedSprites.push_back({task.spriteId, std::move(dummyData), 32, 32});
      continue;
    }

    uint8_t *base = static_cast<uint8_t *>(mapped->data);
    // Header: Signature (4 bytes) + Sprite Count (2 bytes). Offset table starts
    // at byte 6 (for < version 9.6)
    uint32_t offset =
        *reinterpret_cast<uint32_t *>(base + 6 + (task.spriteId - 1) * 4);
    if (offset == 0 || offset >= mapped->size)
      continue;

    std::vector<uint8_t> pixels(32 * 32 * 4, 0); // Transparent Background init
    uint8_t *ptr = base + offset;

    ptr += 3; // Color Key überspringen (meistens Magenta)
    uint16_t pixelDataSize = *reinterpret_cast<uint16_t *>(ptr);
    ptr += 2;

    int writePos = 0, readBytes = 0;
    while (readBytes < pixelDataSize && writePos < 32 * 32) {
      uint16_t transparentPixels = *reinterpret_cast<uint16_t *>(ptr);
      ptr += 2;
      uint16_t coloredPixels = *reinterpret_cast<uint16_t *>(ptr);
      ptr += 2;

      writePos += transparentPixels; // Überspringe transparente Pixel
      for (int i = 0; i < coloredPixels && writePos < 32 * 32; ++i) {
        pixels[writePos * 4 + 0] = *ptr++; // R
        pixels[writePos * 4 + 1] = *ptr++; // G
        pixels[writePos * 4 + 2] = *ptr++; // B
        pixels[writePos * 4 + 3] = 255;    // Alpha Full
        writePos++;
      }
      readBytes += 4 + coloredPixels * 3;
    }

    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_loadedSprites.push_back({task.spriteId, std::move(pixels), 32, 32});
  }
}

void AsyncLoader::update() {
  std::lock_guard<std::mutex> lock(m_resultMutex);
  if (m_loadedSprites.empty())
    return;

  auto *backend = g_gui.GetRenderBackend();
  if (backend) {
    for (const auto &sprite : m_loadedSprites) {
      backend->UpdateAtlas(sprite.width, sprite.height,
                           (void *)sprite.pixelData.data());
    }
  }
  m_loadedSprites.clear();
}

} // namespace RME::Core