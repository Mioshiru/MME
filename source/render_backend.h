#ifndef RME_RENDER_BACKEND_H
#define RME_RENDER_BACKEND_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <algorithm>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>


struct NVGcontext;

namespace RME_Rendering { // Umbenannt, um Konflikte zu vermeiden

struct MapVertex {
  float x, y;
  float u, v;
  uint8_t r, g, b, a;
  float shader_data; // 0: statisch, 1: Wasser, 2: Animation
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

enum class BackendType { OpenGL, Vulkan };

class RenderBackend {
public:
  virtual ~RenderBackend() = default;
  virtual void Initialize(void *windowHandle) = 0;
  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;
  // Übergabe von Geometrie-Daten an das Backend
  virtual void UpdateChunk(uint32_t floorId,
                           const std::vector<MapVertex> &vertices) = 0;
  virtual void UpdateAtlas(uint32_t width, uint32_t height, void *data) = 0;
  virtual void ReloadShaders() = 0;
  virtual void Resize(int w, int h) = 0;
  virtual ::NVGcontext *CreateNanoVGContext() = 0;
};

struct VulkanBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  size_t size;
};

class VulkanBackend : public RME_Rendering::RenderBackend {
public:
  void Initialize(void *windowHandle) override { initVulkan(windowHandle); }

  void UpdateChunk(uint32_t floorId,
                   const std::vector<MapVertex> &vertices) override;

  void UpdateAtlas(uint32_t width, uint32_t height, void *data) override {
    updateTextureDescriptor(data, width, height);
  }

  void BeginFrame() override {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                          imageAvailableSemaphores[currentFrame],
                          VK_NULL_HANDLE, &imageIndex);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
  }

  void EndFrame() override {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void ReloadShaders() override;

  void Resize(int w, int h) override {
    if (w == 0 || h == 0)
      return;
    vkDeviceWaitIdle(device);

    for (auto framebuffer : swapChainFramebuffers) {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (auto imageView : swapChainImageViews) {
      vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    createSwapChain();
    createImageViews();
    createFramebuffers();
  }

  ::NVGcontext *CreateNanoVGContext() override;

  void RenderFrame();
  void SyncPrefabsToGPU();
  void UpdateStagingBuffers();

private:
  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;

  std::map<uint32_t, VulkanBuffer> chunkBuffers;
  std::map<uint32_t, uint32_t> chunkVertexCounts;

  struct PendingChunkUpdate {
    uint32_t floorId;
    std::vector<MapVertex> vertices;
  };
  std::vector<PendingChunkUpdate> pendingChunkUpdates;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  uint32_t currentFrame = 0;
  uint32_t imageIndex = 0;
  const int MAX_FRAMES_IN_FLIGHT = 2;
  bool m_needs_bulk_sync = false;
  ::NVGcontext *nvgContext = nullptr;

  // --- Implementierungen nun ausgelagert in render_backend_init.cpp ---
  void initVulkan(void *windowHandle);
  void createDescriptorSetLayout();
  VulkanBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                            VkMemoryPropertyFlags properties);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void destroyBuffer(VulkanBuffer &buffer);
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void createFramebuffers();
  void createSyncObjects();
  void createInstance();
  void createSurface(void *windowHandle);
  void createRenderPass();
  void createGraphicsPipeline();
  VkShaderModule createShaderModule(const std::vector<char> &code);
  static std::vector<char> readSPIRV(const std::string &filename);
  void createSwapChain();
  void createCommandPool();
  void createCommandBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void updateTextureDescriptor(void *data, uint32_t width, uint32_t height);
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void pickPhysicalDevice();
  void createLogicalDevice();
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
  void createImageViews();
};
} // namespace RME_Rendering

#endif
