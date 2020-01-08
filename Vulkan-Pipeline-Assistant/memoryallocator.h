#ifndef MEMORYALLOCATOR_H
#define MEMORYALLOCATOR_H

#include <vulkan/vulkan.h>
#include <QString>

#include "common.h"

class QVulkanDeviceFunctions;
class QVulkanWindow;
class QImage;
namespace vpa {
    enum class AllocationType {
        BUFFER, IMAGE
    };

    struct Allocation {
        QString name;
        AllocationType type;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        bool isMapped = false;
        union {
            VkBuffer buffer;
            VkImage image;
        };
    };

    class MemoryAllocator {
    public:
        MemoryAllocator(QVulkanDeviceFunctions* deviceFuncs, QVulkanWindow* window, VPAError& err);
        ~MemoryAllocator();

        unsigned char* MapMemory(Allocation& allocation);
        void UnmapMemory(Allocation& allocation);
        // If there is an error in the allocation then resources will be deallocated before return
        VPAError Allocate(VkDeviceSize size, VkBufferUsageFlags usageFlags, QString name, Allocation& allocation);
        VPAError Allocate(VkDeviceSize size, VkImageCreateInfo createInfo, QString name, Allocation& allocation);
        void Deallocate(Allocation& allocation);
        VPAError TransferImageMemory(Allocation& imageAllocation, const VkExtent3D extent, const QImage& image, VkPipelineStageFlags finalStageFlags);

    private:
        QVulkanDeviceFunctions* m_deviceFuncs;
        QVulkanWindow* m_window;
        VkCommandPool m_commandPool;
        VkCommandBuffer m_commandBuffer;
        uint32_t m_transferQueueIdx;
        VkQueue m_transferQueue;
    };
}

#endif // MEMORYALLOCATOR_H
