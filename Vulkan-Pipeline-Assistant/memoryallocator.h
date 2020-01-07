#ifndef MEMORYALLOCATOR_H
#define MEMORYALLOCATOR_H

#include <vulkan/vulkan.h>
#include <QString>

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
        MemoryAllocator(QVulkanDeviceFunctions* deviceFuncs, QVulkanWindow* window);
        ~MemoryAllocator();

        unsigned char* MapMemory(Allocation& allocation);
        void UnmapMemory(Allocation& allocation);
        Allocation Allocate(VkDeviceSize size, VkBufferUsageFlags usageFlags, QString name);
        Allocation Allocate(VkDeviceSize size, VkImageCreateInfo createInfo, QString name);
        void Deallocate(Allocation& allocation);
        void TransferImageMemory(Allocation& imageAllocation, const VkExtent3D extent, const QImage& image, VkPipelineStageFlags finalStageFlags);

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
