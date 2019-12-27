#ifndef MEMORYALLOCATOR_H
#define MEMORYALLOCATOR_H

#include <vulkan/vulkan.h>

class QVulkanDeviceFunctions;
class QVulkanWindow;
namespace vpa {
    enum class AllocationType {
        BUFFER, IMAGE
    };

    struct Allocation {
        AllocationType type;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
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
        Allocation Allocate(VkDeviceSize size, VkBufferUsageFlags usageFlags);
        Allocation Allocate(VkDeviceSize size, VkImageCreateInfo createInfo);
        void Deallocate(Allocation& allocation);

    private:
        QVulkanDeviceFunctions* m_deviceFuncs;
        QVulkanWindow* m_window;
        VkDeviceMemory m_deviceMemory;
    };
}

#endif // MEMORYALLOCATOR_H
