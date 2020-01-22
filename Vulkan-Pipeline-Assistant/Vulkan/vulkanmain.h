#ifndef VULKANMAIN_H
#define VULKANMAIN_H

#include <QVulkanInstance>

#include "reloadflags.h"

class QVulkanWindow;
class QWidget;

namespace vpa {
    struct PipelineConfig;
    class VulkanRenderer;
    class MemoryAllocator;
    class Descriptors;

    constexpr int MaxFrameImages = 3;
    constexpr int MaxFramesInFlight = 2;

    enum class VulkanStateFlags {
        Disabled, Pending, Ok
    };

    struct SwapchainDetails {
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat depthFormat;
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkImage images[MaxFrameImages];
        VkImageView imageViews[MaxFrameImages];
        uint32_t imageCount = 0;
    };

    struct VulkanDetails {
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        uint32_t graphicsQueueIndex = ~0U;
        uint32_t presentQueueIndex = ~0U;
        VkCommandPool mainCommandPool = VK_NULL_HANDLE;
        VkCommandBuffer mainCommandBuffers[MaxFrameImages];
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties physicalDeviceProperties;
        VkPhysicalDeviceFeatures physicalDeviceFeatures;
        QVulkanInstance instance;
        QVulkanFunctions* functions = nullptr;
        QVulkanDeviceFunctions* deviceFunctions = nullptr;
        SwapchainDetails swapchainDetails;
    };

    struct InternalFunctions {
        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
        PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
        PFN_vkQueuePresentKHR vkQueuePresentKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    };

    class VulkanMain final {
        friend class VulkanWindow;
        friend class VulkanRenderer;
    public:
        VulkanMain(QWidget* parent, std::function<void(void)> creationCallback, std::function<void(void)> postInitCallback);
        ~VulkanMain();

        void WritePipelineCache();
        void WritePipelineConfig();
        PipelineConfig& GetConfig();

        void Reload(const ReloadFlags flag);

        Descriptors* GetDescriptors();
        const VkPhysicalDeviceLimits& Limits() const;

    private:
        VPAError Create();
        VPAError CreateVkInstance(QVulkanInstance& instance);
        VPAError CreatePhysicalDevice(VkPhysicalDevice& physicalDevice);
        bool PhysicalDeviceValid();
        VPAError CreateDevice(VkDevice& device);
        VPAError CreateSwapchain(SwapchainDetails& swapchain);

        VPAError ExecuteFrame();
        VPAError AquireImage(uint32_t& imageIdx);
        VPAError SubmitQueue(const uint32_t imageIdx, VkSemaphore signalSemaphores[]);
        VPAError PresentImage(const uint32_t imageIdx, VkSemaphore signalSemaphores[]);

        QVulkanWindow* m_vkWindow;
        VulkanRenderer* m_renderer;
        QWidget* m_container;
        QWidget* m_parent;
        std::function<void(void)> m_creationCallback;
        std::function<void(void)> m_postInitCallback;

        VulkanDetails m_details;
        InternalFunctions m_iFunctions;
        VkSemaphore imagesAvailable[MaxFramesInFlight];
        VkSemaphore renderFinished[MaxFramesInFlight];
        VkFence inFlight[MaxFramesInFlight];
        uint32_t m_frameIndex;

        VulkanStateFlags m_currentState;
    };
}

#endif // VULKANMAIN_H
