#include "memoryallocator.h"

#include <QVulkanDeviceFunctions>
#include <QVulkanWindow>

using namespace vpa;

MemoryAllocator::MemoryAllocator(QVulkanDeviceFunctions* deviceFuncs, QVulkanWindow* window) : m_deviceFuncs(deviceFuncs), m_window(window) { }

MemoryAllocator::~MemoryAllocator() { }

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

    VkResult result;
    result = m_deviceFuncs->vkCreateBuffer(m_window->device(), &bufInfo, nullptr, &allocation.buffer);
    if (result != VK_SUCCESS) qWarning("Failed to create buffer, code %i", result);

    VkMemoryRequirements memReq;
    m_deviceFuncs->vkGetBufferMemoryRequirements(m_window->device(), allocation.buffer, &memReq);

    VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memReq.size, m_window->hostVisibleMemoryIndex() };

    result = m_deviceFuncs->vkAllocateMemory(m_window->device(), &memAllocInfo, nullptr, &allocation.memory);
    if (result != VK_SUCCESS) qWarning("Failed to allocate memory for buffer, code %i", result);

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
