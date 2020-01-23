#include "vulkanmain.h"
#include "vulkanrenderer.h"

#include <QVulkanFunctions>
#include <QLoggingCategory>
#include <QLayout>
#include <QWidget>
#include <QVulkanDeviceFunctions>
#include <QPlatformSurfaceEvent>
#include <QTimer>
#include <QGuiApplication>
#include <qt_windows.h>

#include "common.h"

namespace vpa {
    void VulkanWindow::resizeEvent(QResizeEvent* event) {
        Q_UNUSED(event)
        if(m_main->m_currentState == VulkanState::Ok && !m_handlingResize) m_main->RecreateSwapchain();
    }

#ifdef __clang__
#pragma clang diagnostic push
#if __has_warning("-Wswitch-enum")
    #pragma clang diagnostic ignored "-Wswitch-enum"
#endif
#endif
    bool VulkanWindow::event(QEvent* event) {
        VPAError err  = VPA_OK;
        switch (event->type()) {
        case QEvent::UpdateRequest:
            err = m_main->ExecuteFrame();
            if (err != VPA_OK) {
                if (err.message == "require:SwapchainRecreate") m_main->RecreateSwapchain();
                else m_main->m_currentState = VulkanState::Pending;
            }
            break;
        case QEvent::PlatformSurface:
            if (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
                m_main->Destroy();
            }
            break;
        case QEvent::WindowStateChange:
            qDebug("Windo state changed");
            if (windowState() != Qt::WindowState::WindowMinimized) {
                m_main->RequestUpdate();
            }
            break;
        default:
            break;
        }

        return QWindow::event(event);
    }
#ifdef __clang__
#pragma clang diagnostic pop
#endif

    void VulkanWindow::showEvent(QShowEvent* event) {
        Q_UNUSED(event)
        if(m_main->m_currentState == VulkanState::Ok) m_main->RecreateSwapchain();
    }

    void VulkanWindow::hideEvent(QHideEvent* event) {
        Q_UNUSED(event)
        if (m_main->Details().device != VK_NULL_HANDLE) {
            m_main->Details().deviceFunctions->vkDeviceWaitIdle(m_main->Details().device);
        }
    }

    VulkanMain::VulkanMain(QWidget* parent, std::function<void(void)> creationCallback, std::function<void(void)> postInitCallback)
        : m_renderer(nullptr), m_container(nullptr), m_parent(parent), m_creationCallback(creationCallback), m_postInitCallback(postInitCallback), m_currentState(VulkanState::Pending) {
        m_details.window = nullptr;
        m_renderer = new VulkanRenderer(this, creationCallback, postInitCallback);
        memset(m_renderFinished, 0, sizeof(m_renderFinished));
        memset(m_imagesAvailable, 0, sizeof(m_renderFinished));
        memset(m_inFlight, 0, sizeof(m_renderFinished));
        memset(m_details.swapchainDetails.imageViews, 0, sizeof(m_details.swapchainDetails.imageViews));
        m_currentState = VulkanState::Pending;
        CreateVkInstance(m_details.instance);
        QTimer::singleShot(500, [this]() {
            CreatePhysicalDevice(m_details.physicalDevice);
            CreateDevice(m_details.device);
            VPAError err = Create(false);
            if (err != VPA_OK) {
                qFatal("Error in vulkan main create %s", qPrintable(err.message));
            }
            m_currentState = VulkanState::Ok;
            m_details.window->requestUpdate();
        });
    }

    VulkanMain::~VulkanMain() {
        delete m_renderer;
        delete m_details.window;
        delete m_container;
    }

    void VulkanMain::Destroy() {
        if (m_details.device != VK_NULL_HANDLE) m_details.deviceFunctions->vkDeviceWaitIdle(m_details.device);
        m_frameIndex = 0;
        m_renderer->Release();
        DestroySwapchain();
        for (uint32_t i = 0; i < MaxFramesInFlight; ++i) {
            DESTROY_HANDLE(m_details.device, m_renderFinished[i], m_details.deviceFunctions->vkDestroySemaphore);
            DESTROY_HANDLE(m_details.device, m_imagesAvailable[i], m_details.deviceFunctions->vkDestroySemaphore);
            DESTROY_HANDLE(m_details.device, m_inFlight[i], m_details.deviceFunctions->vkDestroyFence);
        }
        memset(m_renderFinished, 0, sizeof(m_renderFinished));
        memset(m_imagesAvailable, 0, sizeof(m_renderFinished));
        memset(m_inFlight, 0, sizeof(m_renderFinished));

        if (m_details.mainCommandPool != VK_NULL_HANDLE) {
            m_details.deviceFunctions->vkFreeCommandBuffers(m_details.device, m_details.mainCommandPool, uint32_t(MaxFrameImages), m_details.mainCommandBuffers);
            m_details.deviceFunctions->vkDestroyCommandPool(m_details.device, m_details.mainCommandPool, nullptr);
            m_details.mainCommandPool = VK_NULL_HANDLE;
        }
        if (m_details.device != VK_NULL_HANDLE) {
            m_details.deviceFunctions->vkDestroyDevice(m_details.device, nullptr);
            m_details.device = VK_NULL_HANDLE;
        }
        m_currentState = VulkanState::Pending;
    }

    void VulkanMain::DestroySwapchain() {
        m_renderer->CleanUp();
        for (VkImageView view : m_details.swapchainDetails.imageViews) {
            DESTROY_HANDLE(m_details.device, view, m_details.deviceFunctions->vkDestroyImageView);
        }
        memset(m_details.swapchainDetails.imageViews, 0, sizeof(m_details.swapchainDetails.imageViews));
        DESTROY_HANDLE(m_details.device, m_details.swapchainDetails.swapchain, m_iFunctions.vkDestroySwapchainKHR);
    }

    void VulkanMain::RecreateSwapchain() {
        if (m_details.device == VK_NULL_HANDLE) return;

        m_details.deviceFunctions->vkDeviceWaitIdle(m_details.device);
        DestroySwapchain();

        VPAError err = CreateSwapchain(m_details.swapchainDetails);
        if (err != VPA_OK) { // TODO proper VPAError process
            qDebug("Failed to recreate swapchain");
        }
        m_renderer->CreateDefaultObjects();
        Reload(ReloadFlags::RenderPass);
    }

    VPAError VulkanMain::Create(bool destroy) {
        if (destroy) Destroy();
        m_frameIndex = 0;
        VPA_PASS_ERROR(CreateSwapchain(m_details.swapchainDetails));
        VPA_PASS_ERROR(CreateSync());

        m_renderer->Init();

        return VPA_OK;
    }

    VPAError VulkanMain::CreateVkInstance(QVulkanInstance& instance) {
        instance.setLayers(QByteArrayList()
             << "VK_LAYER_GOOGLE_threading"
             << "VK_LAYER_LUNARG_parameter_validation"
             << "VK_LAYER_LUNARG_object_tracker"
             << "VK_LAYER_LUNARG_standard_validation"
             << "VK_LAYER_LUNARG_image"
             << "VK_LAYER_LUNARG_swapchain"
             << "VK_LAYER_GOOGLE_unique_objects");
        instance.setExtensions(QByteArrayList() << VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if (!instance.create()) qFatal("Could not create Vulkan Instance %d", instance.errorCode()); // TODO VPA error

        m_details.functions = m_details.instance.functions();

        m_details.window = new VulkanWindow(this);
        m_details.window->setVulkanInstance(&instance);
        m_details.window->setSurfaceType(QWindow::SurfaceType::VulkanSurface);
        m_details.window->show();

        m_container = QWidget::createWindowContainer(m_details.window);
        m_container->setFocusPolicy(Qt::NoFocus);
        m_parent->layout()->addWidget(m_container);

        m_details.surface = QVulkanInstance::surfaceForWindow(m_details.window);
        if (m_details.surface == VK_NULL_HANDLE) return VPA_CRITICAL("Cannot get vulkan surface");

        return VPA_OK;
    }

    VPAError VulkanMain::CreatePhysicalDevice(VkPhysicalDevice& physicalDevice) {
        uint32_t count = 1;
        VPA_VKCRITICAL_PASS(m_details.functions->vkEnumeratePhysicalDevices(m_details.instance.vkInstance(), &count, nullptr), "Enumerate physical devices count");
        if (count == 0) return VPA_CRITICAL("No physical device detected");
        QVector<VkPhysicalDevice> physicalDevices = QVector<VkPhysicalDevice>(int(count));
        VPA_VKCRITICAL_PASS(m_details.functions->vkEnumeratePhysicalDevices(m_details.instance.vkInstance(), &count, physicalDevices.data()), "Enumerate physical devices");

        physicalDevice = VK_NULL_HANDLE;
        for (VkPhysicalDevice& possiblePhysDevice : physicalDevices) {
            m_details.functions->vkGetPhysicalDeviceProperties(possiblePhysDevice, &m_details.physicalDeviceProperties);
            m_details.functions->vkGetPhysicalDeviceFeatures(possiblePhysDevice, &m_details.physicalDeviceFeatures);
            if (PhysicalDeviceValid()) {
                physicalDevice = possiblePhysDevice;
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE) return VPA_CRITICAL("No suitable physical device found");

        m_iFunctions.vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
                            m_details.instance.getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        m_iFunctions.vkGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
                    m_details.instance.getInstanceProcAddr("vkGetPhysicalDeviceSurfaceSupportKHR"));
        if (!m_iFunctions.vkGetPhysicalDeviceSurfaceCapabilitiesKHR || !m_iFunctions.vkGetPhysicalDeviceSurfaceSupportKHR) {
            return VPA_CRITICAL("Physical device surface queries not available");
        }

        VkPhysicalDeviceMemoryProperties memoryProperties;
        m_details.functions->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                m_details.deviceLocalMemoryIndex = i;
                break;
            }
        }
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if (memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                m_details.hostVisibleMemoryIndex = i;
                break;
            }
        }

        return VPA_OK;
    }

    bool VulkanMain::PhysicalDeviceValid() {
        bool isDiscrete = m_details.physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        bool hasExtensions = true; // TODO more exact extension enumeration and checking
        bool hasQueueFamilies = true; // TODO more exact extension enumeration and checking
        return isDiscrete && hasExtensions && hasQueueFamilies;
    }

    VPAError VulkanMain::CreateDevice(VkDevice& device) {
        uint32_t queueCount = 0;
        m_details.functions->vkGetPhysicalDeviceQueueFamilyProperties(m_details.physicalDevice, &queueCount, nullptr);
        QVector<VkQueueFamilyProperties> queueFamilyProps = QVector<VkQueueFamilyProperties>(int(queueCount));
        m_details.functions->vkGetPhysicalDeviceQueueFamilyProperties(m_details.physicalDevice, &queueCount, queueFamilyProps.data());
        m_details.graphicsQueueIndex = ~0U;
        m_details.presentQueueIndex = ~0U;
        for (int i = 0; i < queueFamilyProps.count(); ++i) {
           const bool supportsPresent = m_details.instance.supportsPresent(m_details.physicalDevice, uint32_t(i), m_details.window);
           if (m_details.graphicsQueueIndex == ~0U
                   && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                   && supportsPresent)
               m_details.graphicsQueueIndex = uint32_t(i);
        }
        if (m_details.graphicsQueueIndex != ~0U) {
           m_details.presentQueueIndex = m_details.graphicsQueueIndex;
        }
        else {
           for (uint32_t i = 0; i < uint32_t(queueFamilyProps.count()); ++i) {
               if (m_details.graphicsQueueIndex == ~0U && (queueFamilyProps[int(i)].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                   m_details.graphicsQueueIndex = i;
               if (m_details.presentQueueIndex == ~0U && m_details.instance.supportsPresent(m_details.physicalDevice, i, m_details.window))
                   m_details.presentQueueIndex = i;
           }
        }

        if (m_details.graphicsQueueIndex == ~0U) return VPA_CRITICAL("No graphics queue family found");
        if (m_details.presentQueueIndex == ~0U) return VPA_CRITICAL("No present queue family found");

        QVector<VkDeviceQueueCreateInfo> queueInfo;
        queueInfo.reserve(2);
        const float prio[] = { 0 };
        VkDeviceQueueCreateInfo addQueueInfo;
        memset(&addQueueInfo, 0, sizeof(addQueueInfo));
        addQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        addQueueInfo.queueFamilyIndex = m_details.graphicsQueueIndex;
        addQueueInfo.queueCount = 1;
        addQueueInfo.pQueuePriorities = prio;
        queueInfo.append(addQueueInfo);
        if (m_details.graphicsQueueIndex != m_details.presentQueueIndex) {
            addQueueInfo.queueFamilyIndex = m_details.presentQueueIndex;
            addQueueInfo.queueCount = 1;
            addQueueInfo.pQueuePriorities = prio;
            queueInfo.append(addQueueInfo);
        }

        QVector<const char *> devExts;
        uint32_t count = 0;
        m_details.functions->vkEnumerateDeviceExtensionProperties(m_details.physicalDevice, nullptr, &count, nullptr);
        QVector<VkExtensionProperties> supportedExtensions = QVector<VkExtensionProperties>(int(count));
        m_details.functions->vkEnumerateDeviceExtensionProperties(m_details.physicalDevice, nullptr, &count, supportedExtensions.data());
        QVulkanInfoVector<QVulkanExtension> exts;
        for (const VkExtensionProperties &prop : supportedExtensions) {
            QVulkanExtension ext;
            ext.name = prop.extensionName;
            ext.version = prop.specVersion;
            exts.append(ext);
        }

        QByteArrayList reqExts =  m_details.instance.extensions();
        reqExts.append("VK_KHR_swapchain");

        QByteArray envExts = qgetenv("QT_VULKAN_DEVICE_EXTENSIONS");
        if (!envExts.isEmpty()) {
            QByteArrayList envExtList =  envExts.split(';');
            for (auto ext : reqExts)
                envExtList.removeAll(ext);
            reqExts.append(envExtList);
        }

        for (const QByteArray &ext : reqExts) {
            if (exts.contains(ext)) devExts.append(ext.constData());
        }

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueInfo.data();
        deviceCreateInfo.queueCreateInfoCount = uint32_t(queueInfo.size());
        deviceCreateInfo.pEnabledFeatures = &m_details.physicalDeviceFeatures;
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr; // TODO enable appropriate layers m_details.instance.layers()
        deviceCreateInfo.enabledExtensionCount = uint32_t(devExts.size());
        deviceCreateInfo.ppEnabledExtensionNames = devExts.constData();
        deviceCreateInfo.pNext = nullptr;
        deviceCreateInfo.flags = 0;
        VPA_VKCRITICAL_PASS(m_details.functions->vkCreateDevice(m_details.physicalDevice, &deviceCreateInfo, nullptr, &device), "Failed to create device");
        m_details.deviceFunctions = m_details.instance.deviceFunctions(m_details.device);

        m_details.deviceFunctions->vkGetDeviceQueue(device, m_details.graphicsQueueIndex, 0, &m_details.graphicsQueue);
        if (m_details.graphicsQueueIndex == m_details.presentQueueIndex) m_details.presentQueue = m_details.graphicsQueue;
        else m_details.deviceFunctions->vkGetDeviceQueue(device, m_details.presentQueueIndex, 0, &m_details.presentQueue);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_details.graphicsQueueIndex;
        VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkCreateCommandPool(device, &poolInfo, nullptr, &m_details.mainCommandPool), "Failed to allocate main command pool");

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_details.mainCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = MaxFrameImages;
        VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkAllocateCommandBuffers(device, &allocInfo, m_details.mainCommandBuffers), "Failed to allocate main command buffers");

        if (!m_iFunctions.vkCreateSwapchainKHR) {
            m_iFunctions.vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(m_details.functions->vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR"));
            m_iFunctions.vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(m_details.functions->vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR"));
            m_iFunctions.vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(m_details.functions->vkGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR"));
            m_iFunctions.vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(m_details.functions->vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR"));
            m_iFunctions.vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(m_details.functions->vkGetDeviceProcAddr(device, "vkQueuePresentKHR"));
        }

        return VPA_OK;
    }

    VPAError VulkanMain::CreateSwapchain(SwapchainDetails& swapchainDetails) {
        VkSurfaceKHR surface = m_details.instance.surfaceForWindow(m_details.window);

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        m_iFunctions.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_details.physicalDevice, surface, &surfaceCapabilities);
        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        swapchainDetails.extent = CalculateExtent();
        swapchainDetails.surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
        swapchainDetails.surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = swapchainDetails.surfaceFormat.format;
        createInfo.imageColorSpace = swapchainDetails.surfaceFormat.colorSpace;
        createInfo.imageExtent = swapchainDetails.extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (m_details.graphicsQueue != m_details.presentQueue) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = &m_details.graphicsQueueIndex;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = swapchainDetails.swapchain;

        VPA_VKCRITICAL_PASS(m_iFunctions.vkCreateSwapchainKHR(m_details.device, &createInfo, nullptr, &swapchainDetails.swapchain), "Failed to create swapchain");

        VkResult err = m_iFunctions.vkGetSwapchainImagesKHR(m_details.device, swapchainDetails.swapchain, &swapchainDetails.imageCount, nullptr);
        if (err != VK_SUCCESS || swapchainDetails.imageCount < 2) return VPA_CRITICAL("Swapchain image count insufficient");
        VPA_VKCRITICAL_PASS(m_iFunctions.vkGetSwapchainImagesKHR(m_details.device, swapchainDetails.swapchain, &swapchainDetails.imageCount, swapchainDetails.images), "Failed to get swapchain images");

        for (uint32_t i = 0; i < swapchainDetails.imageCount; ++i) {
            VkImageViewCreateInfo imageViewCreateInfo = {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = swapchainDetails.images[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = swapchainDetails.surfaceFormat.format;
            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkCreateImageView(m_details.device, &imageViewCreateInfo, nullptr, &swapchainDetails.imageViews[i]), "Could not create swap chain image views");
        }

        swapchainDetails.depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
        for (VkFormat format : { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM }) {
            VkFormatProperties properties;
            m_details.functions->vkGetPhysicalDeviceFormatProperties(m_details.physicalDevice, format, &properties);
            if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT && properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
                swapchainDetails.depthFormat = format;
                break;
            }
        }
        if (swapchainDetails.depthFormat == VkFormat::VK_FORMAT_UNDEFINED) return VPA_CRITICAL("Could not find a valid depth format");

        return VPA_OK;
    }

    VkExtent2D VulkanMain::CalculateExtent() {
        VkExtent2D extent;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        m_iFunctions.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_details.physicalDevice, m_details.surface, &surfaceCapabilities);
        if (surfaceCapabilities.currentExtent.width != ~0U) {
            extent = surfaceCapabilities.currentExtent;
        }
        else {
            extent = { uint32_t(m_details.window->width()), uint32_t(m_details.window->height()) };
        }
        return extent;
    }

    VPAError VulkanMain::CreateSync() {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.pNext = nullptr;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < MaxFramesInFlight; ++i) {
            VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkCreateSemaphore(m_details.device, &semaphoreInfo, nullptr, &m_imagesAvailable[i]), "Create sempahore for swapchain");
            VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkCreateSemaphore(m_details.device, &semaphoreInfo, nullptr, &m_renderFinished[i]), "Create sempahore for swapchain");
            VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkCreateFence(m_details.device, &fenceInfo, nullptr, &m_inFlight[i]), "Create fence for swapchain");
        }

        return VPA_OK;
    }
    VPAError VulkanMain::ExecuteFrame() {
        if (m_currentState == VulkanState::Disabled) {
            return VPA_OK;
        }
        else if (m_currentState == VulkanState::Pending) {
            VPA_PASS_ERROR(Create());
        }

        uint32_t imageIdx;
        VPAError err = AquireImage(imageIdx);

        VkCommandBuffer cmdBuffer = m_details.mainCommandBuffers[imageIdx];

        VkSemaphore signalSemaphores[] = { m_renderFinished[m_frameIndex] };

        VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT), "Reset command buffer");
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkBeginCommandBuffer(cmdBuffer, &beginInfo), "Begin main command buffer for frame index " + QString::number(m_frameIndex));

        m_renderer->RenderFrame(cmdBuffer, imageIdx);

        VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkEndCommandBuffer(cmdBuffer), "End main command buffer");

        SubmitQueue(imageIdx, signalSemaphores);
        PresentImage(imageIdx, signalSemaphores);

        return VPA_OK;
    }

    VPAError VulkanMain::AquireImage(uint32_t& imageIdx) {
        VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkWaitForFences(m_details.device, 1, &m_inFlight[m_frameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max()), "Wait for swapchain fences");
        VkResult result = m_iFunctions.vkAcquireNextImageKHR(m_details.device, m_details.swapchainDetails.swapchain, std::numeric_limits<uint64_t>::max(),
                m_imagesAvailable[m_frameIndex], VK_NULL_HANDLE, &imageIdx);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return VPA_CRITICAL("require:SwapchainRecreate");
        }
        else if (result != VK_SUCCESS) {
            return VPA_CRITICAL("Aquire next image");
        }
        return VPA_OK;
    }

    VPAError VulkanMain::SubmitQueue(const uint32_t imageIdx, VkSemaphore signalSemaphores[]) {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_imagesAvailable[m_frameIndex] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_details.mainCommandBuffers[imageIdx];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        m_details.deviceFunctions->vkResetFences(m_details.device, 1, &m_inFlight[m_frameIndex]);

        VPA_VKCRITICAL_PASS(m_details.deviceFunctions->vkQueueSubmit(m_details.graphicsQueue, 1, &submitInfo, m_inFlight[m_frameIndex]), "Graphics queue submit");

        return VPA_OK;
    }

    VPAError VulkanMain::PresentImage(const uint32_t imageIdx, VkSemaphore signalSemaphores[]) {
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_details.swapchainDetails.swapchain;
        presentInfo.pImageIndices = &imageIdx;

        VkResult result = m_iFunctions.vkQueuePresentKHR(m_details.presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return VPA_CRITICAL("require:SwapchainRecreate");
        }
        else if (result != VK_SUCCESS) {
            return VPA_CRITICAL("Aquire next image");
        }

        m_frameIndex = (m_frameIndex + 1) % MaxFramesInFlight;

        return VPA_OK;
    }

    void VulkanMain::WritePipelineCache() {
        if (m_renderer->WritePipelineCache() == VPA_OK) qDebug("Pipeline cache has been written to test.vpac");
        if (m_renderer->WritePipelineConfig() == VPA_OK) qDebug("Pipeline config has been written to config.vpa");
        if (m_renderer->ReadPipelineConfig() == VPA_OK) qDebug("Pipeline config has been read from config.vpa");
    }

    void VulkanMain::WritePipelineConfig() {
        if (m_renderer->WritePipelineConfig().level == VPAErrorLevel::Ok) {
            qDebug("Pipeline config has been written to config.vpa");
        }
    }

    PipelineConfig& VulkanMain::GetConfig() {
        return m_renderer->GetConfig();
    }

    Descriptors* VulkanMain::GetDescriptors() {
        return m_renderer ? m_renderer->GetDescriptors() : nullptr;
    }

    const VkPhysicalDeviceLimits& VulkanMain::Limits() const {
        return m_details.physicalDeviceProperties.limits;
    }

    void VulkanMain::Reload(const ReloadFlags flag) {
        if (!m_renderer) return;

        VPAError err = m_renderer->Reload(flag);
        if (err != VPA_OK) {
            m_renderer->SetValid(false);
            qDebug(qPrintable(err.message));
        }
        else {
            m_renderer->SetValid(true);
        }
        RequestUpdate();
    }

    void VulkanMain::RequestUpdate() {
        m_details.window->requestUpdate();
    }
}
