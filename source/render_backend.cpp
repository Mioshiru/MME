#include "main.h"
#include "render_backend.h"
#include "gui.h"
#include "map.h"

// #include <nanovg.h>
// #define NANOVG_VULKAN_IMPLEMENTATION
// #include <nanovg_vk.h>

namespace RME_Rendering {

void VulkanBackend::UpdateChunk(uint32_t floorId,
                                const std::vector<MapVertex> &vertices) {
  if (vertices.empty())
    return;

  // Im Vibe-Coding-Stil packen wir die Chunks einfach in eine Warteschlange,
  // statt sie sofort (und blockierend!) auf die GPU zu schieben.
  pendingChunkUpdates.push_back({floorId, vertices});
}

void VulkanBackend::UpdateStagingBuffers() {
  if (pendingChunkUpdates.empty())
    return;

  // 1. Berechne die Gesamtgröße des benötigten Staging-Buffers
  VkDeviceSize totalBufferSize = 0;
  for (const auto &update : pendingChunkUpdates) {
    totalBufferSize += sizeof(MapVertex) * update.vertices.size();
  }

  // 2. Erstelle EINEN großen Staging-Buffer für den gesamten Upload-Batch
  VulkanBuffer stagingBuffer =
      createBuffer(totalBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // 3. Mappe den gesamten Speicher und kopiere die Vertices für jeden Chunk ein
  void *data;
  vkMapMemory(device, stagingBuffer.memory, 0, totalBufferSize, 0, &data);

  VkDeviceSize currentOffset = 0;
  for (const auto &update : pendingChunkUpdates) {
    VkDeviceSize chunkSize = sizeof(MapVertex) * update.vertices.size();

    memcpy(static_cast<char *>(data) + currentOffset, update.vertices.data(),
           (size_t)chunkSize);

    if (chunkBuffers.count(update.floorId)) {
      destroyBuffer(chunkBuffers[update.floorId]);
    }

    chunkBuffers[update.floorId] = createBuffer(
        chunkSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    chunkVertexCounts[update.floorId] =
        static_cast<uint32_t>(update.vertices.size());

    currentOffset += chunkSize;
  }
  vkUnmapMemory(device, stagingBuffer.memory);

  // 4. High-Speed Transfer auf der GPU asynchron ausführen (Sammel-Befehl)
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  currentOffset = 0;
  for (const auto &update : pendingChunkUpdates) {
    VkDeviceSize chunkSize = sizeof(MapVertex) * update.vertices.size();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = currentOffset;
    copyRegion.dstOffset = 0;
    copyRegion.size = chunkSize;

    vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer,
                    chunkBuffers[update.floorId].buffer, 1, &copyRegion);

    currentOffset += chunkSize;
  }

  endSingleTimeCommands(commandBuffer);

  // 5. Aufräumen
  destroyBuffer(stagingBuffer);
  pendingChunkUpdates.clear();
}

void VulkanBackend::SyncPrefabsToGPU() {
  // Wenn der Prefab-Batch beendet ist, flushen wir alle gesammelten VBOs
  // auf einmal zur GPU. Verhindert hunderte winzige Speicher-Allokationen!
  if (!pendingChunkUpdates.empty()) {
    UpdateStagingBuffers();
  }
}

void VulkanBackend::RenderFrame() {
  // Nutzt VBO Ring Buffering aus map_drawer.cpp
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);

  vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                        &imageIndex);

  // Optimierte Prefab-Anbindung
  Map &currentMap = g_gui.GetCurrentMap();
  if (currentMap.isBatching()) {
    // Während des Batch-Einfügens (Prefabs) sammeln wir nur die Dirty-Regions
    // und unterdrücken das Recording der Command Buffer für diesen Frame.
    m_needs_bulk_sync = true;
  } else if (m_needs_bulk_sync) {
    // Batch abgeschlossen: Starte High-Speed Transfer
    SyncPrefabsToGPU();
    m_needs_bulk_sync = false;
  } else {
    // Normaler Frame-Update
    UpdateStagingBuffers();
  }

  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  // Submit Graphics Queue
  VkSubmitInfo submitInfo{};
  // ... setup and submit ...
}

NVGcontext *VulkanBackend::CreateNanoVGContext() {
  if (nvgContext)
    return nvgContext;

  // Vorerst deaktiviert, da nanovg_vk.h im Standard-VCPKG fehlt.
  // VKNVGCreateInfo createInfo{};
  // createInfo.device = device;
  // createInfo.gpu = physicalDevice;
  // createInfo.renderpass = renderPass;
  // createInfo.cmdPool = commandPool;
  // createInfo.cmdQueue = graphicsQueue;
  // createInfo.imageCount = MAX_FRAMES_IN_FLIGHT;

  // // NanoVG VBOs und Shader in die Vulkan Queue einklinken
  // nvgContext = nvgCreateVk(createInfo, NVG_ANTIALIAS | NVG_STENCIL_STROKES,
  //                          graphicsQueue);

  return nvgContext;
}

} // namespace RME_Rendering
