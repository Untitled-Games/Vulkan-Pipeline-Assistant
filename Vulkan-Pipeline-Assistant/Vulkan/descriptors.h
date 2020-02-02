#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include <QVector>
#include <QHash>
#include <QMap>
#include <QMatrix4x4>

#include "../common.h"
#include "spirvresource.h"
#include "memoryallocator.h"

namespace vpa {
    class VulkanMain;

    struct DescriptorInfo {
        uint32_t set = 0;
        uint32_t binding = 0;
        VkDescriptorSetLayoutBinding layoutBinding;
        VkWriteDescriptorSet writeSet;
        SpvGroupName type;
        Allocation allocation;
        SpvResource* resource = nullptr;
    };

    struct BufferInfo {
        VkDescriptorBufferInfo bufferInfo;
        VkBufferUsageFlags usage = 0;
        DescriptorInfo descriptor;
    };

    struct ImageInfo {
        VkDescriptorImageInfo imageInfo;
        VkImageView view = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        DescriptorInfo descriptor;
    };

    struct PushConstantInfo {
        ShaderStage stage;
        SpvResource* resource = nullptr;
        QVector<unsigned char> data;
    };

    enum class BuiltInSets : uint32_t {
        OutputPostPass = 0, //, EnvironmentMap //, ShadowMap
        Count_
    };

    class Descriptors final {
        static constexpr float NearPlane = 1.0f;
        static constexpr float FarPlane = 100.0f;
    public:
        Descriptors(VulkanMain* main, QVulkanDeviceFunctions* deviceFuncs, MemoryAllocator* allocator, uint32_t attachmentCount,
                    const DescriptorLayoutMap& layoutMap, const QVector<SpvResource*>& pushConstants, VkPhysicalDeviceLimits limits, VPAError& err);
        ~Descriptors();

        const QHash<uint32_t, QVector<BufferInfo>>& Buffers() const { return m_buffers; }
        const QHash<uint32_t, QVector<ImageInfo>>& Images() const { return m_images; }
        const QMap<ShaderStage, PushConstantInfo>& PushConstants() const { return m_pushConstants; }

        unsigned char* MapBufferPointer(uint32_t set, int index);
        void UnmapBufferPointer(uint32_t set, int index);
        void LoadImage(const uint32_t set, const int index, const QString name);
        unsigned char* PushConstantData(ShaderStage stage);
        // CompletePushConstantData should be called after modifying any push constant data to update the display.
        void CompletePushConstantData();

        void CmdBindSets(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const;
        void CmdPushConstants(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const;

        const QVector<VkDescriptorSetLayout>& DescriptorSetLayouts() const;
        const QVector<VkPushConstantRange>& PushConstantRanges() const;

        VkDescriptorSetLayout* BuiltInSetLayout(BuiltInSets set) { return &m_builtInLayouts[int(set)]; }
        VkDescriptorSet BuiltInSet(BuiltInSets set) const { return m_builtInSets[int(set)]; }

        static const QMatrix4x4 DefaultModelMatrix();
        static const QMatrix4x4 DefaultViewMatrix();
        static const QMatrix4x4 DefaultProjectionMatrix();
        static const QMatrix4x4 DefaultMVPMatrix();

    private:
        VPAError EnumerateShaderRequirements(QVector<VkDescriptorPoolSize>& poolSizes, QVector<VkDescriptorSetLayout>& layouts, uint32_t& setCount, const DescriptorLayoutMap& layoutMap, const QVector<SpvResource*>& pushConstants);
        VPAError EnumerateBuiltInRequirements(QVector<VkDescriptorPoolSize>& poolSizes, QVector<VkDescriptorSetLayout>& layouts, uint32_t& setCount, uint32_t attachmentCount);

        VPAError Validate(size_t numSets, const QVector<VkDescriptorPoolSize>& poolSizes);
        VPAError BuildDescriptors(QSet<uint32_t>& sets, QVector<VkDescriptorPoolSize>& poolSizes, const DescriptorLayoutMap& layoutMap);
        VPAError CreateBuffer(DescriptorInfo& descriptor, const SpvResource* resource, BufferInfo& info);
        VPAError CreateImage(ImageInfo& imageInfo, const QString& name, bool writeSet);
        void WriteShaderDescriptors();

        PushConstantInfo CreatePushConstant(SpvResource* resource);
        void BuildPushConstantRanges();

        VkPipelineStageFlags StageFlagsToPipelineFlags(VkShaderStageFlags stageFlags);
        void DestroyImage(ImageInfo& imageInfo);

        VkImageCreateInfo MakeImageCreateInfo(const SpvImageType* type, uint32_t width, uint32_t height, uint32_t depth) const;

        VulkanMain* m_main;
        QVulkanDeviceFunctions* m_deviceFuncs;

        MemoryAllocator* m_allocator;
        QHash<uint32_t, QVector<BufferInfo>> m_buffers;
        QHash<uint32_t, QVector<ImageInfo>> m_images;
        QMap<ShaderStage, PushConstantInfo> m_pushConstants;

        QVector<VkPushConstantRange> m_pushConstantRanges;
        QHash<uint32_t, int> m_descriptorSetIndexMap;
        QVector<VkDescriptorSet> m_descriptorSets;
        QVector<VkDescriptorSetLayout> m_descriptorLayouts;
        QVector<VkDescriptorSet> m_builtInSets;
        QVector<VkDescriptorSetLayout> m_builtInLayouts;
        VkDescriptorPool m_descriptorPool;

        VkPhysicalDeviceLimits m_limits;

        static double s_aspectRatio;
    };
}

#endif // DESCRIPTORS_H
