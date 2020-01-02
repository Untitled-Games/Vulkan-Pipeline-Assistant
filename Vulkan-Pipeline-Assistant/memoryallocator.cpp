#include "memoryallocator.h"

#include <QVulkanDeviceFunctions>
#include <QVulkanWindow>
#include <QImage>

#include "vulkanmain.h"

using namespace vpa;

MemoryAllocator::MemoryAllocator(QVulkanDeviceFunctions* deviceFuncs, QVulkanWindow* window) : m_deviceFuncs(deviceFuncs), m_window(window) {
    QVulkanFunctions* funcs = window->vulkanInstance()->functions();
    uint32_t queueCount = 0;
    funcs->vkGetPhysicalDeviceQueueFamilyProperties(window->physicalDevice(), &queueCount, nullptr);
    QVector<VkQueueFamilyProperties> queueFamilies(queueCount);
    funcs->vkGetPhysicalDeviceQueueFamilyProperties(window->physicalDevice(), &queueCount, queueFamilies.data());
    for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); ++i) {
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

    FATAL_VKRESULT(m_deviceFuncs->vkCreateCommandPool(window->device(), &poolInfo, nullptr, &m_commandPool), "command pool allocation");

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    FATAL_VKRESULT(m_deviceFuncs->vkAllocateCommandBuffers(window->device(), &allocInfo, &m_commandBuffer), "command buffer allocation");
}

MemoryAllocator::~MemoryAllocator() {
    m_deviceFuncs->vkDestroyCommandPool(m_window->device(), m_commandPool, nullptr);
}

unsigned char* MemoryAllocator::MapMemory(Allocation& allocation) {
    unsigned char* dataPtr = nullptr;
    WARNING_VKRESULT(m_deviceFuncs->vkMapMemory(m_window->device(), allocation.memory, 0, allocation.size, 0, reinterpret_cast<void **>(&dataPtr)), qPrintable("map memory for allocation '" + allocation.name + "'"));
    return dataPtr;
}

void MemoryAllocator::UnmapMemory(Allocation& allocation) {
    m_deviceFuncs->vkUnmapMemory(m_window->device(), allocation.memory);
}

Allocation MemoryAllocator::Allocate(VkDeviceSize size, VkBufferUsageFlags usageFlags, QString name) {
    Allocation allocation;
    allocation.name = name;
    allocation.type = AllocationType::BUFFER;
    allocation.size = size;

    VkBufferCreateInfo bufInfo = {};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = size;
    bufInfo.usage = usageFlags;

    const VkPhysicalDeviceLimits& deviceLimits = m_window->physicalDeviceProperties()->limits;

    WARNING_VKRESULT(m_deviceFuncs->vkCreateBuffer(m_window->device(), &bufInfo, nullptr, &allocation.buffer), qPrintable("create buffer for allocation '" + allocation.name + "'"));

    VkMemoryRequirements memReq;
    m_deviceFuncs->vkGetBufferMemoryRequirements(m_window->device(), allocation.buffer, &memReq);

    VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memReq.size, m_window->hostVisibleMemoryIndex() };

    WARNING_VKRESULT(m_deviceFuncs->vkAllocateMemory(m_window->device(), &memAllocInfo, nullptr, &allocation.memory), qPrintable("allocate buffer memory for allocation '" + allocation.name + "'"));

    WARNING_VKRESULT(m_deviceFuncs->vkBindBufferMemory(m_window->device(), allocation.buffer, allocation.memory, 0), qPrintable("bind buffer memory for allocation '" + allocation.name + "'"));

    return allocation;
}

Allocation MemoryAllocator::Allocate(VkDeviceSize size, VkImageCreateInfo createInfo, QString name) {
    Allocation allocation;
    allocation.name = name;
    allocation.type = AllocationType::IMAGE;
    allocation.size = size;

    WARNING_VKRESULT(m_deviceFuncs->vkCreateImage(m_window->device(), &createInfo, nullptr, &allocation.image), qPrintable("create image for allocation '" + allocation.name + "'"));

    VkMemoryRequirements memReq;
    m_deviceFuncs->vkGetImageMemoryRequirements(m_window->device(), allocation.image, &memReq);

    VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memReq.size, m_window->deviceLocalMemoryIndex() };

    WARNING_VKRESULT(m_deviceFuncs->vkAllocateMemory(m_window->device(), &memAllocInfo, nullptr, &allocation.memory), qPrintable("allocate image memory for allocation '" + allocation.name + "'"));

    WARNING_VKRESULT(m_deviceFuncs->vkBindImageMemory(m_window->device(), allocation.image, allocation.memory, 0), qPrintable("bind image memory for allocation '" + allocation.name + "'"));

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

void MemoryAllocator::TransferImageMemory(Allocation& imageAllocation, const VkExtent3D extent, const QImage& image) {
    Allocation stagingAllocation = Allocate(imageAllocation.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, "staging_buffer");

    VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout layout;
    m_deviceFuncs->vkGetImageSubresourceLayout(m_window->device(), imageAllocation.image, &subres, &layout);

    unsigned char* stagingData = MapMemory(stagingAllocation);
    for (int y = 0; y < image.height(); ++y) {
        const unsigned char* line = image.constScanLine(y);
        memcpy(stagingData, line, image.width() * 4);
        stagingData += layout.rowPitch;
    }
    UnmapMemory(stagingAllocation);

    VkBufferImageCopy copyRegion = {};
    copyRegion.imageExtent = extent;
    copyRegion.imageOffset = { 0, 0, 0 };
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = imageAllocation.image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr };
    WARNING_VKRESULT(m_deviceFuncs->vkBeginCommandBuffer(m_commandBuffer, &beginInfo), qPrintable("begin transfer command buffer for allocation '" + imageAllocation.name + "'"));

    m_deviceFuncs->vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    m_deviceFuncs->vkCmdCopyBufferToImage(m_commandBuffer, stagingAllocation.buffer, imageAllocation.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // TODO decide how to include storage image with  | VK_ACCESS_SHADER_WRITE_BIT
    m_deviceFuncs->vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_SHADER_STAGE_ALL_GRAPHICS, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    WARNING_VKRESULT(m_deviceFuncs->vkEndCommandBuffer(m_commandBuffer), qPrintable("end transfer command buffer for allocation '" + imageAllocation.name + "'"));

    VkSubmitInfo submitInfo = { };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    WARNING_VKRESULT(m_deviceFuncs->vkQueueSubmit(m_transferQueue, 1, &submitInfo, VK_NULL_HANDLE), qPrintable("transfer queue submit for allocation '" + imageAllocation.name + "'"));
    m_deviceFuncs->vkQueueWaitIdle(m_transferQueue);

    Deallocate(stagingAllocation);
}
