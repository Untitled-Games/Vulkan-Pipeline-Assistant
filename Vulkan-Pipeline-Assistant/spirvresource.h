#ifndef SPIRVRESOURCE_H
#define SPIRVRESOURCE_H

#include <vulkan/vulkan.h>
#include <QString>
#include <QPair>

namespace spirv_cross {
    struct Resource;
    class Compiler;
}
namespace vpa {
    enum class ShaderStage {
        VERTEX, FRAGMENT, TESS_CONTROL, TESS_EVAL, GEOMETRY, count_
    };

    inline VkShaderStageFlagBits StageToVkStageFlag(ShaderStage stage) {
        switch (stage) {
        case ShaderStage::VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::TESS_CONTROL:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderStage::TESS_EVAL:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case ShaderStage::GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        default:
            assert(false);
        }
    }

    enum class SpirvResourceType {
        INPUT_ATTRIBUTE, UNIFORM_BUFFER, STORAGE_BUFFER, SAMPLER_IMAGE, STORAGE_IMAGE, PUSH_CONSTANT
    };

    struct SpirvResource {
        QString name;
        size_t size;
        ShaderStage stage;
        SpirvResourceType resourceType;
        spirv_cross::Resource* spirvResource;
        spirv_cross::Compiler* compiler;

        bool operator==(const SpirvResource& other) {
            return name == other.name && size == other.size && resourceType == other.resourceType;
        }
        bool operator!=(const SpirvResource& other) {
            return !operator==(other);
        }
    };

    using DescriptorLayoutMap = QHash<QPair<uint32_t, uint32_t>, SpirvResource>;
}

#endif // SPIRVRESOURCE_H
