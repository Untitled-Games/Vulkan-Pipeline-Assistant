#ifndef VULKANMAIN_H
#define VULKANMAIN_H

#include <QVulkanInstance>
#include <QWindow>

#include "reloadflags.h"

class QWidget;

namespace vpa {
    struct PipelineConfig;
    class VulkanRenderer;
    class MemoryAllocator;
    class Descriptors;
    class VulkanWindow;
    class VulkanMain;

    constexpr uint32_t MaxFrameImages = 3;
    constexpr uint32_t MaxFramesInFlight = 2;

    enum class VulkanState {
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
        VkExtent2D extent;
    };

    struct VulkanDetails {
        VulkanWindow* window = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        uint32_t graphicsQueueIndex = ~0U;
        uint32_t presentQueueIndex = ~0U;

        VkCommandPool mainCommandPool = VK_NULL_HANDLE;
        VkCommandBuffer mainCommandBuffers[MaxFrameImages];
        uint32_t hostVisibleMemoryIndex;
        uint32_t deviceLocalMemoryIndex;

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
        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
        PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
        PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
    };

    class VulkanWindow : public QWindow {
    public:
        VulkanWindow(VulkanMain* main) : m_main(main), m_handlingResize(false) { }
        ~VulkanWindow() override = default;
        void resizeEvent(QResizeEvent* event) override;
        bool event(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void HandlingResize(bool handling) { m_handlingResize = handling; }

    private:
        VulkanMain* m_main;
        bool m_handlingResize;
    };

    class VulkanMain final {
        friend class VulkanWindow;
        friend class VulkanRenderer;
    public:
        VulkanMain(QWidget* parent, std::function<void(void)> creationCallback);
        ~VulkanMain();

        void WritePipelineCache();
        void WritePipelineConfig();
        PipelineConfig& GetConfig();

        void Reload(const ReloadFlags flag);
        void RequestUpdate();
        void RecreateSwapchain();

        Descriptors* GetDescriptors();
        const VkPhysicalDeviceLimits& Limits() const;
        const VulkanDetails& Details() const { return m_details; }
        VkDevice Device() const { return m_details.device; }

        VulkanState State() const { return m_currentState; }

    private:
        void Destroy();
        void DestroySwapchain();
        VPAError Create(bool destroy = true);
        VPAError CreateVkInstance(QVulkanInstance& instance);

        VPAError CreatePhysicalDevice(VkPhysicalDevice& physicalDevice);
        bool PhysicalDeviceValid(VkPhysicalDevice& physicalDevice);
        bool HasExtensions(VkPhysicalDevice& physicalDevice);
        bool HasQueueFamilies(VkPhysicalDevice& physicalDevice);
        bool SupportsBasicFeatures(VkPhysicalDevice& physicalDevice);

        VPAError CreateDevice(VkDevice& device);
        void CalculateLayers(VkDeviceCreateInfo& createInfo);

        VPAError CreateSwapchain(SwapchainDetails& swapchain);
        VkExtent2D CalculateExtent();
        VPAError CreateSync();

        VPAError ExecuteFrame();
        VPAError AquireImage(uint32_t& imageIdx);
        VPAError SubmitQueue(const uint32_t imageIdx, VkSemaphore signalSemaphores[]);
        VPAError PresentImage(const uint32_t imageIdx, VkSemaphore signalSemaphores[]);

        VulkanRenderer* m_renderer;
        QWidget* m_container;
        QWidget* m_parent;
        std::function<void(void)> m_creationCallback;

        QByteArrayList m_requiredExtensions;
        QVector<const char *> m_deviceExtensions;

        VulkanDetails m_details;
        InternalFunctions m_iFunctions;
        VkSemaphore m_imagesAvailable[MaxFramesInFlight];
        VkSemaphore m_renderFinished[MaxFramesInFlight];
        VkFence m_inFlight[MaxFramesInFlight];
        uint32_t m_frameIndex;

        VulkanState m_currentState;

        static const QVector<const char*> LayerNames;
    };
}

#endif // VULKANMAIN_H
