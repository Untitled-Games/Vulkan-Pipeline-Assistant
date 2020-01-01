#include "descriptors.h"

#include <QHash>
#include <QVulkanWindow>
#include <QVulkanDeviceFunctions>

#include "vulkanmain.h"

using namespace vpa;

Descriptors::Descriptors(QVulkanWindow* window, QVulkanDeviceFunctions* deviceFuncs, MemoryAllocator* allocator, DescriptorLayoutMap& layoutMap, QVector<SpirvResource> pushConstants)
    : m_window(window), m_deviceFuncs(deviceFuncs), m_allocator(allocator) {
    QSet<uint32_t> sets;
    QVector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1},
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
    };

    for (auto key : layoutMap.keys()) {
        DescriptorInfo descriptor = {};
        descriptor.set = key.first;
        descriptor.binding = key.second;
        descriptor.type = layoutMap[key].resourceType;
        if (descriptor.type == SpirvResourceType::UNIFORM_BUFFER || descriptor.type == SpirvResourceType::STORAGE_BUFFER) {
            m_buffers[key.first].push_back(CreateBuffer(descriptor, layoutMap[key]));
            if (descriptor.type == SpirvResourceType::UNIFORM_BUFFER) {
                poolSizes[0].descriptorCount++;
                poolSizes[1].descriptorCount++;
            }
            else {
                poolSizes[2].descriptorCount++;
                poolSizes[3].descriptorCount++;
            }
        }
        else if (descriptor.type == SpirvResourceType::SAMPLER_IMAGE || descriptor.type == SpirvResourceType::STORAGE_IMAGE) {
            m_images[key.first].push_back(CreateImage(descriptor, layoutMap[key]));
            if (descriptor.type == SpirvResourceType::SAMPLER_IMAGE) {
                poolSizes[4].descriptorCount++;
            }
            else {
                poolSizes[5].descriptorCount++;
            }
        }
        else {
            qWarning("Unsupported resource in shader.");
        }
        sets.insert(key.first);
    }

    for (auto res : pushConstants) {
        m_pushConstants[res.stage] = CreatePushConstant(res);
    }

    BuildPushConstantRanges();

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.pNext = nullptr;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = uint32_t(sets.size());

    FATAL_VKRESULT(m_deviceFuncs->vkCreateDescriptorPool(m_window->device(), &poolInfo, nullptr, &m_descriptorPool), "create descriptor pool");

    m_descriptorSets.resize(sets.size());
    m_descriptorLayouts.resize(sets.size());
    for (int i = 0; i < sets.size(); ++i) {
        uint32_t set = *(sets.begin() + i);
        QVector<VkDescriptorSetLayoutBinding> bindings;
        for (auto& buf : m_buffers[set]) {
            bindings.push_back(buf.descriptor.layoutBinding);
            buf.descriptor.descriptorSet = &m_descriptorSets[i];
        }
        for (auto& img : m_images[set]) {
            bindings.push_back(img.descriptor.layoutBinding);
            img.descriptor.descriptorSet = &m_descriptorSets[i];
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = uint32_t(bindings.size());
        layoutInfo.pBindings = bindings.data();
        layoutInfo.pNext = nullptr;
        FATAL_VKRESULT(m_deviceFuncs->vkCreateDescriptorSetLayout(m_window->device(), &layoutInfo, nullptr, &m_descriptorLayouts[i]), qPrintable("create descriptor set layout for set " + QString(set)));
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = uint32_t(m_descriptorLayouts.size());
    allocInfo.pSetLayouts = m_descriptorLayouts.data();

    FATAL_VKRESULT(m_deviceFuncs->vkAllocateDescriptorSets(m_window->device(), &allocInfo, &m_descriptorSets[0]), "allocate all descriptor sets");

    QVector<VkWriteDescriptorSet> writes;
    for (auto& buffers : m_buffers) {
        for (BufferInfo& buffer : buffers) {
            buffer.descriptor.writeSet = {};
            buffer.descriptor.writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            buffer.descriptor.writeSet.dstSet = *buffer.descriptor.descriptorSet;
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
            image.descriptor.writeSet.dstSet = *image.descriptor.descriptorSet;
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

Descriptors::~Descriptors() {
    for (auto buffers : m_buffers) {
        for (BufferInfo& buffer : buffers) {
            m_allocator->Deallocate(buffer.descriptor.allocation);
        }
    }
    for (auto images : m_images) {
        for (ImageInfo& image : images) {
            m_allocator->Deallocate(image.descriptor.allocation);
        }
    }
    for (auto& layout : m_descriptorLayouts) {
        DESTROY_HANDLE(m_window->device(), layout, m_deviceFuncs->vkDestroyDescriptorSetLayout);
    }
    DESTROY_HANDLE(m_window->device(), m_descriptorPool, m_deviceFuncs->vkDestroyDescriptorPool);
}

void Descriptors::WriteBufferData(uint32_t set, int index, size_t size, size_t offset, void* data) {
    Allocation& allocation = m_buffers[set][index].descriptor.allocation;
    assert(size <= allocation.size);
    if (data != nullptr) {
        unsigned char* dataPtr = m_allocator->MapMemory(allocation);
        memcpy((dataPtr + offset), data, size);
        m_allocator->Deallocate(allocation);
    }
}

void Descriptors::LoadImage(int index, QString name) {
    // TODO create image, image view, and resuse sampler.
    // Load image data from file. Supported formats will be png (stb) and dds (nv_dds).
}

void Descriptors::WritePushConstantData(ShaderStage stage, size_t size, void* data) {
    memcpy(m_pushConstants[stage].data.data(), data, size);
}

void Descriptors::CmdBindSets(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const {
    // TODO dynamic offset support
    m_deviceFuncs->vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, m_descriptorSets.size(), m_descriptorSets.data(), 0, nullptr);
}

void Descriptors::CmdPushConstants(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout) const {
    // Map stores stages in order, therefore offset assumes later stages data is always offset by earlier stages. TODO consider arbitrary order.
    uint32_t offset = 0;
    for (auto& pushConstant : m_pushConstants) {
        m_deviceFuncs->vkCmdPushConstants(cmdBuf, pipelineLayout, StageToVkStageFlag(pushConstant.stage), offset, pushConstant.data.size(), pushConstant.data.data());
        offset += pushConstant.data.size();
    }
}

const QVector<VkDescriptorSetLayout>& Descriptors::DescriptorSetLayouts() const {
    return m_descriptorLayouts;
}

const QVector<VkPushConstantRange>& Descriptors::PushConstantRanges() const {
    return m_pushConstantRanges;
}

BufferInfo Descriptors::CreateBuffer(DescriptorInfo& dinfo, SpirvResource resource) {
    BufferInfo info = {};
    info.usage = dinfo.type == SpirvResourceType::UNIFORM_BUFFER ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.descriptor = dinfo;
    info.descriptor.allocation = m_allocator->Allocate(resource.size, info.usage, resource.name);

    info.bufferInfo = {};
    info.bufferInfo.buffer = info.descriptor.allocation.buffer;
    info.bufferInfo.offset = 0;
    info.bufferInfo.range = info.descriptor.allocation.size;

    info.descriptor.layoutBinding = {};
    info.descriptor.layoutBinding.binding = info.descriptor.binding;
    info.descriptor.layoutBinding.descriptorCount = 1;
    info.descriptor.layoutBinding.descriptorType = dinfo.type == SpirvResourceType::UNIFORM_BUFFER ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    info.descriptor.layoutBinding.binding = info.descriptor.binding;
    info.descriptor.layoutBinding.pImmutableSamplers = nullptr;
    info.descriptor.layoutBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

    return info;
}

ImageInfo Descriptors::CreateImage(DescriptorInfo& dinfo, SpirvResource resource) {
    ImageInfo info = {};
    info.descriptor = dinfo;

     // TODO allow better customisation for images
    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.usage = dinfo.type == SpirvResourceType::SAMPLER_IMAGE ? VK_IMAGE_USAGE_SAMPLED_BIT : VK_IMAGE_USAGE_STORAGE_BIT;
    createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.flags = 0;
    createInfo.pNext = nullptr;

    createInfo.extent = {2, 2, 1}; // TODO width, height, depth
    createInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    info.descriptor.allocation = m_allocator->Allocate(resource.size, createInfo, resource.name);
    return info;
}

PushConstantInfo Descriptors::CreatePushConstant(SpirvResource& resource) {
    PushConstantInfo info = {};
    info.stage = resource.stage;
    info.data.resize(resource.size);
    return info;
}

void Descriptors::BuildPushConstantRanges() {
    uint32_t offset = 0;
    for (auto& pushConstant : m_pushConstants) {
        VkPushConstantRange range = {};
        range.size = pushConstant.data.size();
        range.offset = offset;
        range.stageFlags = StageToVkStageFlag(pushConstant.stage);
        offset += pushConstant.data.size();
        m_pushConstantRanges.push_back(range);
    }
}
