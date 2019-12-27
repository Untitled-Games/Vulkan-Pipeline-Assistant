#ifndef SPIRVRESOURCE_H
#define SPIRVRESOURCE_H

#include <Lib/spirv-cross/spirv_cross.hpp>
#include <QString>

namespace vpa {
    enum class ShaderStage {
        VETREX, FRAGMENT, TESS_CONTROL, TESS_EVAL, GEOMETRY, count_
    };

    enum class SpirvResourceType {
        INPUT_ATTRIBUTE, UNIFORM_BUFFER, STORAGE_BUFFER, SAMPLER_IMAGE, STORAGE_IMAGE, PUSH_CONSTANT
    };

    struct SpirvResource {
        QString name;
        size_t size;
        SpirvResourceType resourceType;
        SPIRV_CROSS_NAMESPACE::Resource spirvResource;
        SPIRV_CROSS_NAMESPACE::Compiler* compiler;

        bool operator==(const SpirvResource& other) {
            return name == other.name && size == other.size && resourceType == other.resourceType;
        }
        bool operator!=(const SpirvResource& other) {
            return !operator==(other);
        }
    };
}

#endif // SPIRVRESOURCE_H
