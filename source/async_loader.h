#ifndef RME_CORE_ASYNC_LOADER_H
#define RME_CORE_ASYNC_LOADER_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace RME::Core {

struct SpriteLoadTask {
  uint32_t spriteId;
  std::string filePath;
};

struct LoadedSprite {
  uint32_t spriteId;
  std::vector<uint8_t> pixelData;
  uint32_t width;
  uint32_t height;
};

class AsyncLoader {
public:
  AsyncLoader();
  ~AsyncLoader();

  void start();
  void stop();

  // Wird von den Sprite-Klassen gecalled
  void queueSpriteLoad(uint32_t spriteId, const std::string &filePath = "");
  // Wird vom Main-Render-Loop einmal pro Frame gecalled (Pusht die Texturen zur
  // GPU)
  void update();

private:
  void workerLoop();

  struct MappedFile {
    void *data = nullptr;
    size_t size = 0;
    void *hFile = nullptr;
    void *hMapping = nullptr;
  };
  std::unordered_map<std::string, MappedFile> m_mappedFiles;
  MappedFile *getMappedFile(const std::string &path);
  void closeMappedFiles();

  std::thread m_workerThread;
  std::mutex m_queueMutex;
  std::condition_variable m_condition;
  std::queue<SpriteLoadTask> m_pendingTasks;

  std::mutex m_resultMutex;
  std::vector<LoadedSprite> m_loadedSprites;

  std::atomic<bool> m_running;
};

} // namespace RME::Core
#endif