#include "descriptors.h"

#include <QHash>
#include <QVulkanWindow>
#include <QVulkanDeviceFunctions>
#include <algorithm>

#include "common.h"

namespace vpa {
    double Descriptors::s_aspectRatio = 0.0;

    Descriptors::Descriptors(QVulkanWindow* window, QVulkanDeviceFunctions* deviceFuncs, MemoryAllocator* allocator,
                             const DescriptorLayoutMap& layoutMap, const QVector<SpvResource*>& pushConstants, VkPhysicalDeviceLimits limits, VPAError& err)
        : m_window(window), m_deviceFuncs(deviceFuncs), m_allocator(allocator), m_descriptorPool(VK_NULL_HANDLE), m_limits(limits) {
        s_aspectRatio = m_window->width() / m_window->height();

        QVector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
        };

        uint32_t setCount = 0;
        EnumerateShaderRequirements(poolSizes, m_descriptorLayouts, setCount, layoutMap, pushConstants);
        EnumerateBuiltInRequirements(poolSizes, m_builtInLayouts, setCount);

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = 0;
        poolInfo.pNext = nullptr;
        poolInfo.poolSizeCount = uint32_t(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = setCount;

        VPA_VKCRITICAL_CTOR_PASS(m_deviceFuncs->vkCreateDescriptorPool(m_window->device(), &poolInfo, nullptr, &m_descriptorPool), "create descriptor pool", err)

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = uint32_t(m_descriptorLayouts.size());
        allocInfo.pSetLayouts = m_descriptorLayouts.data();

        m_descriptorSets.resize(m_descriptorLayouts.size());
        VPA_VKCRITICAL_CTOR_PASS(m_deviceFuncs->vkAllocateDescriptorSets(m_window->device(), &allocInfo, &m_descriptorSets[0]), "allocate shader descriptor sets", err)

        allocInfo.descriptorSetCount = uint32_t(m_builtInLayouts.size());
        allocInfo.pSetLayouts = m_builtInLayouts.data();

        m_builtInSets.resize(m_builtInLayouts.size());
        VPA_VKCRITICAL_CTOR_PASS(m_deviceFuncs->vkAllocateDescriptorSets(m_window->device(), &allocInfo, &m_builtInSets[0]), "allocate built in descriptor sets", err)

        WriteShaderDescriptors();

        err = VPA_OK;
    }

    Descriptors::~Descriptors() {
        for (auto buffers : m_buffers) {
            for (BufferInfo& buffer : buffers) {
                m_allocator->Deallocate(buffer.descriptor.allocation);
            }
        }
        for (auto images : m_images) {
            for (ImageInfo& image : images) {
                DestroyImage(image);
            }
        }
        for (auto& layout : m_descriptorLayouts) {
            DESTROY_HANDLE(m_window->device(), layout, m_deviceFuncs->vkDestroyDescriptorSetLayout)
        }
        for (auto& layout : m_builtInLayouts) {
            DESTROY_HANDLE(m_window->device(), layout, m_deviceFuncs->vkDestroyDescriptorSetLayout)
        }
        DESTROY_HANDLE(m_window->device(), m_descriptorPool, m_deviceFuncs->vkDestroyDescriptorPool)
    }

    unsigned char* Descriptors::MapBufferPointer(uint32_t set, int index) {
        m_deviceFuncs->vkDeviceWaitIdle(m_window->device());
        Allocation& allocation = m_buffers[set][index].descriptor.allocation;
        return m_allocator->MapMemory(allocation);
    }

    void Descriptors::UnmapBufferPointer(uint32_t set, int index) {
        Allocation& allocation = m_buffers[set][index].descriptor.allocation;
        m_allocator->UnmapMemory(allocation);
    }

    void Descriptors::LoadImage(const uint32_t set, const int index, const QString name) {
        m_deviceFuncs->vkDeviceWaitIdle(m_window->device());
        ImageInfo& imageInfo = m_images[set][index];
        DestroyImage(imageInfo);
        CreateImage(imageInfo, name, true);
    }

    unsigned char* Descriptors::PushConstantData(ShaderStage stage) {
        return m_pushConstants[stage].data.data();
    }

    void Descriptors::CmdBindSets(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const {
        // TODO dynamic offset support
        m_deviceFuncs->vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, uint32_t(m_descriptorSets.size()), m_descriptorSets.data(), 0, nullptr);
    }

    void Descriptors::CmdPushConstants(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const {
        // Map stores stages in order, therefore offset assumes later stages data is always offset by earlier stages. TODO consider arbitrary order.
        uint32_t offset = 0;
        for (auto& pushConstant : m_pushConstants) {
            m_deviceFuncs->vkCmdPushConstants(cmdBuf, pipelineLayout, StageToVkStageFlag(pushConstant.stage), offset, uint32_t(pushConstant.data.size()), pushConstant.data.data());
            offset += uint32_t(pushConstant.data.size());
        }
    }

    const QVector<VkDescriptorSetLayout>& Descriptors::DescriptorSetLayouts() const {
        return m_descriptorLayouts;
    }

    const QVector<VkPushConstantRange>& Descriptors::PushConstantRanges() const {
        return m_pushConstantRanges;
    }

    VPAError Descriptors::EnumerateShaderRequirements(QVector<VkDescriptorPoolSize>& poolSizes, QVector<VkDescriptorSetLayout>& layouts, uint32_t& setCount, const DescriptorLayoutMap& layoutMap, const QVector<SpvResource*>& pushConstants) {

        QSet<uint32_t> setIndices;
        if (!layoutMap.empty()) {
            BuildDescriptors(setIndices, poolSizes, layoutMap);
        }
        for (auto res : pushConstants) {
            m_pushConstants[reinterpret_cast<const SpvPushConstantGroup*>(res->group)->stage] = CreatePushConstant(res);
        }
        BuildPushConstantRanges();

        setCount += uint32_t(setIndices.size());

        Validate(uint32_t(setIndices.size()), poolSizes);

        if (!setIndices.empty()) {
            QVector<uint32_t> setIndicesVec(setIndices.begin(), setIndices.end());
            std::sort(setIndicesVec.begin(), setIndicesVec.end(), std::less<uint32_t>());

            layouts.resize(setIndicesVec.size());
            for (int i = 0; i < setIndicesVec.size(); ++i) {
                uint32_t set = *(setIndicesVec.begin() + i);
                m_descriptorSetIndexMap[set] = i; // TODO think about this issue
                QVector<VkDescriptorSetLayoutBinding> bindings;
                for (auto& buf : m_buffers[set]) {
                    bindings.push_back(buf.descriptor.layoutBinding);
                }
                for (auto& img : m_images[set]) {
                    bindings.push_back(img.descriptor.layoutBinding);
                }

                VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = uint32_t(bindings.size());
                layoutInfo.pBindings = bindings.data();
                layoutInfo.pNext = nullptr;
                VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateDescriptorSetLayout(m_window->device(), &layoutInfo, nullptr, &layouts[i]), qPrintable("create descriptor set layout for set " + QString(set)))
            }
        }

        return VPA_OK;
    }

    VPAError Descriptors::EnumerateBuiltInRequirements(QVector<VkDescriptorPoolSize>& poolSizes, QVector<VkDescriptorSetLayout>& layouts, uint32_t& setCount) {

        setCount += uint32_t(BuiltInSets::Count_);
        layouts.resize(int(BuiltInSets::Count_));

        // ------- Depth post pass ------
        VkDescriptorSetLayoutBinding depthLayoutBinding = {};
        depthLayoutBinding.binding = 0;
        depthLayoutBinding.descriptorCount = 1;
        depthLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        depthLayoutBinding.pImmutableSamplers = nullptr;
        depthLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        poolSizes[4].descriptorCount++;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &depthLayoutBinding;
        layoutInfo.pNext = nullptr;

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateDescriptorSetLayout(m_window->device(), &layoutInfo, nullptr, &layouts[int(BuiltInSets::DepthPostPass)]), "create descriptor set layout for depth set ")

        return VPA_OK;
    }

    VPAError Descriptors::Validate(size_t numSets, const QVector<VkDescriptorPoolSize>& poolSizes) {
        VPA_PASS_ERROR(VPAAssert(numSets <= m_limits.maxBoundDescriptorSets, "setLayoutCount must be less than or equal to VkPhysicalDeviceLimits::maxBoundDescriptorSets"));

        VPA_PASS_ERROR(VPAAssert((poolSizes[4].descriptorCount - 1) <= m_limits.maxDescriptorSetSamplers, "Num samplers beyond maxDescriptorSetSamplers, https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineLayoutCreateInfo.html"));
        VPA_PASS_ERROR(VPAAssert((poolSizes[0].descriptorCount - 1) <= m_limits.maxDescriptorSetUniformBuffers, "Num uniform buffers beyond maxDescriptorSetUniformBuffers, https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineLayoutCreateInfo.html"));
        VPA_PASS_ERROR(VPAAssert((poolSizes[2].descriptorCount - 1) <= m_limits.maxDescriptorSetStorageBuffers, "Num storage buffers beyond maxDescriptorSetStorageBuffers, https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineLayoutCreateInfo.html"));
        VPA_PASS_ERROR(VPAAssert((poolSizes[4].descriptorCount - 1) <= m_limits.maxDescriptorSetSampledImages, "Num sampled images beyond maxDescriptorSetSampledImages, https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineLayoutCreateInfo.html"));
        VPA_PASS_ERROR(VPAAssert((poolSizes[5].descriptorCount - 1) <= m_limits.maxDescriptorSetStorageImages, "Num storage images beyond maxDescriptorSetStorageImages, https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineLayoutCreateInfo.html"));

        return VPA_OK;
    }

    VPAError Descriptors::BuildDescriptors(QSet<uint32_t>& sets, QVector<VkDescriptorPoolSize>& poolSizes, const DescriptorLayoutMap& layoutMap) {
        for (auto key : layoutMap.keys()) {
            DescriptorInfo descriptor = {};
            descriptor.set = key.first;
            descriptor.binding = key.second;
            descriptor.resource = layoutMap[key];
            descriptor.type = layoutMap[key]->group->Group();
            if (descriptor.type == SpvGroupName::UniformBuffer || descriptor.type == SpvGroupName::StorageBuffer) {
                BufferInfo bufferInfo = {};
                VPA_PASS_ERROR(CreateBuffer(descriptor, layoutMap[key], bufferInfo));
                m_buffers[key.first].push_back(bufferInfo);
                if (descriptor.type == SpvGroupName::UniformBuffer) {
                    poolSizes[0].descriptorCount++;
                    poolSizes[1].descriptorCount++;
                }
                else {
                    poolSizes[2].descriptorCount++;
                    poolSizes[3].descriptorCount++;
                }
            }
            else if (descriptor.type == SpvGroupName::Image) {
                ImageInfo info = {};
                info.descriptor = descriptor;
                VPA_PASS_ERROR(CreateImage(info, TEXDIR"default.png", false));
                m_images[key.first].push_back(info);
                if (reinterpret_cast<const SpvImageType*>(descriptor.resource->type)->sampled) {
                    poolSizes[4].descriptorCount++;
                }
                else {
                    poolSizes[5].descriptorCount++;
                }
            }
            else {
                return VPA_WARN("Unsupported resource in shader.");
            }
            sets.insert(key.first);
        }
        return VPA_OK;
    }

    VPAError Descriptors::CreateBuffer(DescriptorInfo& descriptor, const SpvResource* resource, BufferInfo& info) {
        info = {};
        info.usage = descriptor.type == SpvGroupName::UniformBuffer ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.descriptor = descriptor;
        VPA_PASS_ERROR(m_allocator->Allocate(reinterpret_cast<const SpvStructType*>(resource->type)->size, info.usage, resource->name, info.descriptor.allocation));

        info.bufferInfo = {};
        info.bufferInfo.buffer = info.descriptor.allocation.buffer;
        info.bufferInfo.offset = 0;
        info.bufferInfo.range = info.descriptor.allocation.size;

        info.descriptor.layoutBinding = {};
        info.descriptor.layoutBinding.binding = info.descriptor.binding;
        info.descriptor.layoutBinding.descriptorCount = 1;
        info.descriptor.layoutBinding.descriptorType = descriptor.type == SpvGroupName::UniformBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        info.descriptor.layoutBinding.binding = info.descriptor.binding;
        info.descriptor.layoutBinding.pImmutableSamplers = nullptr;
        info.descriptor.layoutBinding.stageFlags = reinterpret_cast<const SpvDescriptorGroup*>(resource->group)->stageFlags;

        return VPA_OK;
    }

    VPAError Descriptors::CreateImage(ImageInfo& imageInfo, const QString& name, bool writeSet) {
        QImage image(name);
        if (image.isNull()) return VPA_CRITICAL("Failed to load image " + name);
        image = image.convertToFormat(QImage::Format_RGBA8888);

        VkImageCreateInfo createInfo = MakeImageCreateInfo(reinterpret_cast<const SpvImageType*>(imageInfo.descriptor.resource->type),
                                                           uint32_t(image.width()), uint32_t(image.height()), 1);

        size_t size = size_t(image.width()) * size_t(image.height()) * 4;
        VPA_PASS_ERROR(m_allocator->Allocate(size, createInfo, imageInfo.descriptor.resource->name, imageInfo.descriptor.allocation));

        VkShaderStageFlags shaderStageFlags = reinterpret_cast<const SpvDescriptorGroup*>(imageInfo.descriptor.resource->group)->stageFlags;
        VkPipelineStageFlags finalStageFlags = StageFlagsToPipelineFlags(shaderStageFlags);

        VPAError err = VPA_OK;
        err = m_allocator->TransferImageMemory(imageInfo.descriptor.allocation, createInfo.extent, image, finalStageFlags);
        if (err != VPA_OK) {
            DestroyImage(imageInfo);
            return err;
        }

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = imageInfo.descriptor.allocation.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = createInfo.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = createInfo.mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = createInfo.arrayLayers;

        VPA_VKCRITICAL(m_deviceFuncs->vkCreateImageView(m_window->device(), &viewInfo, nullptr, &imageInfo.view),
                         qPrintable("create image view for allocation '" + imageInfo.descriptor.allocation.name + "'"), err)
        if (err != VPA_OK) {
            DestroyImage(imageInfo);
            return err;
        }

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 0.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;
        VPA_VKCRITICAL(m_deviceFuncs->vkCreateSampler(m_window->device(), &samplerInfo, nullptr, &imageInfo.sampler),
                         qPrintable("create sampler for allocation '" + imageInfo.descriptor.allocation.name + "'"), err)
        if (err != VPA_OK) {
            DestroyImage(imageInfo);
            return err;
        }

        imageInfo.imageInfo = {};
        imageInfo.imageInfo.imageView = imageInfo.view;
        imageInfo.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageInfo.sampler = imageInfo.sampler;

        imageInfo.descriptor.layoutBinding = {};
        imageInfo.descriptor.layoutBinding.binding = imageInfo.descriptor.binding;
        imageInfo.descriptor.layoutBinding.descriptorCount = 1;
        imageInfo.descriptor.layoutBinding.descriptorType = reinterpret_cast<const SpvImageType*>(imageInfo.descriptor.resource->type)->sampled ?
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        imageInfo.descriptor.layoutBinding.pImmutableSamplers = nullptr;
        imageInfo.descriptor.layoutBinding.stageFlags = shaderStageFlags;

        if (writeSet) {
            QVector<VkWriteDescriptorSet> writes;
            imageInfo.descriptor.writeSet = {};
            imageInfo.descriptor.writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageInfo.descriptor.writeSet.dstSet = m_descriptorSets[m_descriptorSetIndexMap[imageInfo.descriptor.set]];
            imageInfo.descriptor.writeSet.dstBinding = imageInfo.descriptor.binding;
            imageInfo.descriptor.writeSet.dstArrayElement = 0;
            imageInfo.descriptor.writeSet.descriptorType = imageInfo.descriptor.layoutBinding.descriptorType;
            imageInfo.descriptor.writeSet.descriptorCount = 1;
            imageInfo.descriptor.writeSet.pImageInfo = &imageInfo.imageInfo;
            writes.push_back(imageInfo.descriptor.writeSet);
            m_deviceFuncs->vkUpdateDescriptorSets(m_window->device(), uint32_t(writes.size()), writes.data(), 0, nullptr);
        }
        return VPA_OK;
    }

    void Descriptors::WriteShaderDescriptors() {
        QVector<VkWriteDescriptorSet> writes;
        for (auto& buffers : m_buffers) {
            for (BufferInfo& buffer : buffers) {
                buffer.descriptor.writeSet = {};
                buffer.descriptor.writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                buffer.descriptor.writeSet.dstSet = m_descriptorSets[m_descriptorSetIndexMap[buffer.descriptor.set]];
                buffer.descriptor.writeSet.dstBinding = buffer.descriptor.binding;
                buffer.descriptor.writeSet.dstArrayElement = 0;
                buffer.descriptor.writeSet.descriptorType = buffer.descriptor.layoutBinding.descriptorType;
                buffer.descriptor.writeSet.descriptorCount = 1;
                buffer.descriptor.writeSet.pBufferInfo = &buffer.bufferInfo;
                writes.push_back(buffer.descriptor.writeSet);
            }
        }

        for (auto& images : m_images) {
            for (ImageInfo& image : images) {
                image.descriptor.writeSet = {};
                image.descriptor.writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                image.descriptor.writeSet.dstSet = m_descriptorSets[m_descriptorSetIndexMap[image.descriptor.set]];
                image.descriptor.writeSet.dstBinding = image.descriptor.binding;
                image.descriptor.writeSet.dstArrayElement = 0;
                image.descriptor.writeSet.descriptorType = image.descriptor.layoutBinding.descriptorType;
                image.descriptor.writeSet.descriptorCount = 1;
                image.descriptor.writeSet.pImageInfo = &image.imageInfo;
                writes.push_back(image.descriptor.writeSet);
            }
        }

        m_deviceFuncs->vkUpdateDescriptorSets(m_window->device(), uint32_t(writes.size()), writes.data(), 0, nullptr);
    }

    PushConstantInfo Descriptors::CreatePushConstant(SpvResource* resource) {
        PushConstantInfo info = {};
        info.stage = reinterpret_cast<const SpvPushConstantGroup*>(resource->group)->stage;
        info.data.resize(int(reinterpret_cast<const SpvStructType*>(resource->type)->size));
        info.resource = resource;
        return info;
    }

    void Descriptors::BuildPushConstantRanges() {
        uint32_t offset = 0;
        for (auto& pushConstant : m_pushConstants) {
            VkPushConstantRange range = {};
            range.size = uint32_t(pushConstant.data.size());
            range.offset = offset;
            range.stageFlags = StageToVkStageFlag(pushConstant.stage);
            offset += uint32_t(pushConstant.data.size());
            m_pushConstantRanges.push_back(range);
        }
    }

    VkPipelineStageFlags Descriptors::StageFlagsToPipelineFlags(VkShaderStageFlags stageFlags) {
        if (stageFlags & VK_SHADER_STAGE_ALL) return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        if (stageFlags & VK_SHADER_STAGE_ALL_GRAPHICS) return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

        VkPipelineStageFlags pipelineFlags = 0;
        if (stageFlags & VK_SHADER_STAGE_VERTEX_BIT) pipelineFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        if (stageFlags & VK_SHADER_STAGE_FRAGMENT_BIT) pipelineFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        if (stageFlags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) pipelineFlags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        if (stageFlags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) pipelineFlags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        if (stageFlags & VK_SHADER_STAGE_GEOMETRY_BIT) pipelineFlags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;

        return pipelineFlags;
    }

    void Descriptors::DestroyImage(ImageInfo& imageInfo) {
        DESTROY_HANDLE(m_window->device(), imageInfo.sampler, m_deviceFuncs->vkDestroySampler)
        DESTROY_HANDLE(m_window->device(), imageInfo.view, m_deviceFuncs->vkDestroyImageView)
        m_allocator->Deallocate(imageInfo.descriptor.allocation);
    }

    const QMatrix4x4 Descriptors::DefaultModelMatrix() {
        QMatrix4x4 model;
        model.setToIdentity();
        model.scale(0.1f, 0.1f, 0.1f);
        return model;
    }

    const QMatrix4x4 Descriptors::DefaultViewMatrix() {
        QMatrix4x4 view;
        view.lookAt(QVector3D(0.0, 10.0, 20.0), QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));
        return view;
    }

    const QMatrix4x4 Descriptors::DefaultProjectionMatrix() {
        QMatrix4x4 projection;
        projection.perspective(45.0f, float(s_aspectRatio), NearPlane, FarPlane);
        projection.data()[5] *= -1;
        return projection;
    }

    const QMatrix4x4 Descriptors::DefaultMVPMatrix() {
        return DefaultProjectionMatrix() * DefaultViewMatrix() * DefaultModelMatrix();
    }

    VkImageCreateInfo Descriptors::MakeImageCreateInfo(const SpvImageType* type, uint32_t width, uint32_t height, uint32_t depth) const {
        VkImageCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.usage = type->sampled ? VK_IMAGE_USAGE_SAMPLED_BIT : VK_IMAGE_USAGE_STORAGE_BIT;
        createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.flags = 0;
        createInfo.pNext = nullptr;

        createInfo.extent = { width, height, depth };
        createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.mipLevels = 1;
        createInfo.arrayLayers = 1;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return createInfo;
    }
}
