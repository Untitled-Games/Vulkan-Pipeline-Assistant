#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#define TEXDIR "../../Resources/Textures/"

#include <QVector>
#include <QHash>
#include <QMap>
#include <QMatrix4x4>

#include "spirvresource.h"
#include "memoryallocator.h"

namespace vpa {
    struct DescriptorInfo {
        uint32_t set;
        uint32_t binding;
        VkDescriptorSetLayoutBinding layoutBinding;
        VkWriteDescriptorSet writeSet;
        SpvGroupName type;
        Allocation allocation;
        SpvResource* resource;
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
        DescriptorInfo descriptor;
    };

    struct PushConstantInfo {
        ShaderStage stage;
        SpvResource* resource;
        QVector<unsigned char> data;
    };

    class Descriptors {
    public:
        Descriptors(QVulkanWindow* window, QVulkanDeviceFunctions* deviceFuncs, MemoryAllocator* allocator,
                    const DescriptorLayoutMap& layoutMap, const QVector<SpvResource*>& pushConstants);
        ~Descriptors();

        const QHash<uint32_t, QVector<BufferInfo>>& Buffers() const { return m_buffers; }
        const QHash<uint32_t, QVector<ImageInfo>>& Images() const { return m_images; }
        const QMap<ShaderStage, PushConstantInfo>& PushConstants() const { return m_pushConstants; }

        unsigned char* MapBufferPointer(uint32_t set, int index);
        void UnmapBufferPointer(uint32_t set, int index);
        void LoadImage(const uint32_t set, const int index, const QString name);
        unsigned char* PushConstantData(ShaderStage stage);

        void CmdBindSets(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const;
        void CmdPushConstants(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const;

        const QVector<VkDescriptorSetLayout>& DescriptorSetLayouts() const;
        const QVector<VkPushConstantRange>& PushConstantRanges() const;

        static const QMatrix4x4 DefaultModelMatrix();
        static const QMatrix4x4 DefaultViewMatrix();
        static const QMatrix4x4 DefaultProjectionMatrix();
        static const QMatrix4x4 DefaultMVPMatrix();

    private:
        void BuildDescriptors(QSet<uint32_t>& sets, QVector<VkDescriptorPoolSize>& poolSizes, const DescriptorLayoutMap& layoutMap);
        BufferInfo CreateBuffer(DescriptorInfo& descriptor, const SpvResource* resource);
        void CreateImage(ImageInfo& imageInfo, const QString& name, bool writeSet);
        PushConstantInfo CreatePushConstant(SpvResource* resource);
        void BuildPushConstantRanges();
        VkPipelineStageFlags StageFlagsToPipelineFlags(VkShaderStageFlags stageFlags);
        void DestroyImage(ImageInfo& imageInfo);

        QVulkanWindow* m_window;
        QVulkanDeviceFunctions* m_deviceFuncs;

        MemoryAllocator* m_allocator;
        QHash<uint32_t, QVector<BufferInfo>> m_buffers;
        QHash<uint32_t, QVector<ImageInfo>> m_images;
        QMap<ShaderStage, PushConstantInfo> m_pushConstants;

        QVector<VkPushConstantRange> m_pushConstantRanges;
        QHash<uint32_t, int> m_descriptorSetIndexMap;
        QVector<VkDescriptorSet> m_descriptorSets;
        QVector<VkDescriptorSetLayout> m_descriptorLayouts;
        VkDescriptorPool m_descriptorPool;

        static double s_aspectRatio;
    };
}

#endif // DESCRIPTORS_H
