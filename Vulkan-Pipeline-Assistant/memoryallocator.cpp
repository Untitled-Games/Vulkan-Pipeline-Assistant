#include "memoryallocator.h"

#include <QVulkanDeviceFunctions>
#include <QVulkanWindow>

using namespace vpa;

MemoryAllocator::MemoryAllocator(QVulkanDeviceFunctions* deviceFuncs, QVulkanWindow* window) : m_deviceFuncs(deviceFuncs), m_window(window) {
    VkQueueFamilyProperties queueProperties;
    QVulkanFunctions* funcs = window->vulkanInstance()->functions();
    uint32_t queueCount = 0;
    funcs->vkGetPhysicalDeviceQueueFamilyProperties(window->physicalDevice(), &queueCount, nullptr);
    QVector<VkQueueFamilyProperties> queueFamilies(queueCount);
    funcs->vkGetPhysicalDeviceQueueFamilyProperties(window->physicalDevice(), &queueCount, queueFamilies.data());
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            m_transferQueueIdx = i;
            break;
        }
    }
    deviceFuncs->vkGetDeviceQueue(window->device(), m_transferQueueIdx, 0, &m_transferQueue);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_transferQueueIdx;
    poolInfo.flags = 0;

    VkResult result = m_deviceFuncs->vkCreateCommandPool(window->device(), &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) qFatal("Failed to allocate command pool for memory allocator!");

    VkCommandBufferAllocateInfo allocInfo = {};//m_commandBuffer
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    result = m_deviceFuncs->vkAllocateCommandBuffers(window->device(), &allocInfo, &m_commandBuffer);
    if (result != VK_SUCCESS) qFatal("Failed to allocate command buffer for transfer!");
}

MemoryAllocator::~MemoryAllocator() {
    m_deviceFuncs->vkDestroyCommandPool(m_window->device(), m_commandPool, nullptr);
}

unsigned char* MemoryAllocator::MapMemory(Allocation& allocation) {
    unsigned char* dataPtr = nullptr;
    VkResult result = m_deviceFuncs->vkMapMemory(m_window->device(), allocation.memory, 0, allocation.size, 0, reinterpret_cast<void **>(&dataPtr));
    if (result != VK_SUCCESS) qWarning("Failed to map buffer memory, code %i", result);
    return dataPtr;
}

void MemoryAllocator::UnmapMemory(Allocation& allocation) {
    m_deviceFuncs->vkUnmapMemory(m_window->device(), allocation.memory);
}

Allocation MemoryAllocator::Allocate(VkDeviceSize size, VkBufferUsageFlags usageFlags) {
    Allocation allocation;
    allocation.type = AllocationType::BUFFER;
    allocation.size = size;

    VkBufferCreateInfo bufInfo = {};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = size;
    bufInfo.usage = usageFlags;

    const VkPhysicalDeviceLimits& deviceLimits = m_window->physicalDeviceProperties()->limits;

    VkResult result;
    result = m_deviceFuncs->vkCreateBuffer(m_window->device(), &bufInfo, nullptr, &allocation.buffer);
    if (result != VK_SUCCESS) qWarning("Failed to create buffer, code %i", result);

    VkMemoryRequirements memReq;
    m_deviceFuncs->vkGetBufferMemoryRequirements(m_window->device(), allocation.buffer, &memReq);

    VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memReq.size, m_window->hostVisibleMemoryIndex() };

    result = m_deviceFuncs->vkAllocateMemory(m_window->device(), &memAllocInfo, nullptr, &allocation.memory);
    if (result != VK_SUCCESS) qWarning("Failed to allocate memory for buffer, code %i", result);

    result = m_deviceFuncs->vkBindBufferMemory(m_window->device(), allocation.buffer, allocation.memory, 0);
    if (result != VK_SUCCESS) qWarning("Failed to bind buffer to memory, code %i", result);

    return allocation;
}

// TODO implement
Allocation MemoryAllocator::Allocate(VkDeviceSize size, VkImageCreateInfo createInfo) {
    Allocation allocation;
    allocation.type = AllocationType::IMAGE;
    allocation.size = 0;
    return allocation;
}

void MemoryAllocator::Deallocate(Allocation& allocation) {
    if (allocation.type == AllocationType::BUFFER) {
        m_deviceFuncs->vkDestroyBuffer(m_window->device(), allocation.buffer, nullptr);
        allocation.buffer = VK_NULL_HANDLE;
    }
    else {
        m_deviceFuncs->vkDestroyImage(m_window->device(), allocation.image, nullptr);
        allocation.image = VK_NULL_HANDLE;
    }
    m_deviceFuncs->vkFreeMemory(m_window->device(), allocation.memory, nullptr);
    allocation.memory = VK_NULL_HANDLE;
    allocation.size = 0;
}
