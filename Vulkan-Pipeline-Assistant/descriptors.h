#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include <QVector>
#include <QHash>
#include <QMap>

#include "spirvresource.h"
#include "memoryallocator.h"

namespace vpa {
    struct DescriptorInfo {
        uint32_t set;
        uint32_t binding;
        VkDescriptorSetLayoutBinding layoutBinding;
        VkWriteDescriptorSet writeSet;
        SpirvResourceType type;
        Allocation allocation;
        VkDescriptorSet* descriptorSet;
    };

    struct BufferInfo {
        VkDescriptorBufferInfo bufferInfo;
        VkBufferUsageFlags usage;
        DescriptorInfo descriptor;
    };

    struct ImageInfo {
        VkDescriptorImageInfo imageInfo;
        VkImageView view;
        VkSampler sampler;
        SpirvResource resource;
        DescriptorInfo descriptor;
    };

    struct PushConstantInfo {
        ShaderStage stage;
        QVector<unsigned char> data;
    };

    class Descriptors {
    public:
        Descriptors(QVulkanWindow* window, QVulkanDeviceFunctions* deviceFuncs, MemoryAllocator* allocator,
                    DescriptorLayoutMap& layoutMap, QVector<SpirvResource> pushConstants);
        ~Descriptors();

        int BufferCount() { return m_buffers.size(); }
        int ImageCount() { return m_images.size(); }
        int PushConstantCount() { return m_pushConstants.size(); }

        const QHash<uint32_t, QVector<BufferInfo>>& Buffers() const { return m_buffers; }
        const QHash<uint32_t, QVector<ImageInfo>>& Images() const { return m_images; }
        const QMap<ShaderStage, PushConstantInfo>& PushConstants() const { return m_pushConstants; }

        void WriteBufferData(uint32_t set, int index, size_t size, size_t offset, void* data);
        void LoadImage(const uint32_t set, const int index, const QString name);
        void WritePushConstantData(ShaderStage stage, size_t size, void* data);

        void CmdBindSets(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const;
        void CmdPushConstants(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const;

        const QVector<VkDescriptorSetLayout>& DescriptorSetLayouts() const;
        const QVector<VkPushConstantRange>& PushConstantRanges() const;

    private:
        BufferInfo CreateBuffer(DescriptorInfo& descriptor, SpirvResource resource);
        void CreateImage(ImageInfo& imageInfo, const QString& name);
        PushConstantInfo CreatePushConstant(SpirvResource& resource);
        void BuildPushConstantRanges();

        void DestroyImage(ImageInfo& imageInfo);

        QVulkanWindow* m_window;
        QVulkanDeviceFunctions* m_deviceFuncs;

        MemoryAllocator* m_allocator;
        QHash<uint32_t, QVector<BufferInfo>> m_buffers;
        QHash<uint32_t, QVector<ImageInfo>> m_images;
        QMap<ShaderStage, PushConstantInfo> m_pushConstants;

        QVector<VkPushConstantRange> m_pushConstantRanges;
        QVector<VkDescriptorSet> m_descriptorSets;
        QVector<VkDescriptorSetLayout> m_descriptorLayouts;
        VkDescriptorPool m_descriptorPool;
    };
}

#endif // DESCRIPTORS_H
