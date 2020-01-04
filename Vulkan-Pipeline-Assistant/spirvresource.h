#ifndef SPIRVRESOURCE_H
#define SPIRVRESOURCE_H

#include <vulkan/vulkan.h>
#include <QString>
#include <QPair>
#include <QVector>

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

    enum class SpvGroupname {
        INPUT_ATTRIBUTE, UNIFORM_BUFFER, STORAGE_BUFFER, SAMPLER_IMAGE, STORAGE_IMAGE, PUSH_CONSTANT, IMAGE // TODO remove sampler/storage
    };

    struct SpvGroup {
        virtual ~SpvGroup() { }
        virtual SpvGroupname Group() const = 0;
        virtual bool operator==(const SpvGroup* other) const = 0;
    };

    struct SpvInputAttribGroup : public SpvGroup {
        uint32_t location;

        SpvInputAttribGroup(uint32_t location) : location(location) { }
        SpvGroupname Group() const override {
            return SpvGroupname::INPUT_ATTRIBUTE;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    location == ((const SpvInputAttribGroup*)(other))->location;
        }
    };

    struct SpvPushConstantGroup : public SpvGroup {
        ShaderStage stage;

        SpvPushConstantGroup(ShaderStage stage) : stage(stage) { }
        SpvGroupname Group() const override {
            return SpvGroupname::PUSH_CONSTANT;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    stage == ((const SpvPushConstantGroup*)(other))->stage;
        }
    };

    struct SpvDescriptorGroup : public SpvGroup {
        uint32_t set;
        uint32_t binding;
        VkShaderStageFlags stageFlags; // Can have multiple stages, so compress to a flag
        SpvGroupname group;

        SpvDescriptorGroup(uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags, SpvGroupname group)
            : set(set), binding(binding), stageFlags(stageFlags), group(group) { }
        void AddStageFlag(VkShaderStageFlagBits flag) { stageFlags |= flag; }
        SpvGroupname Group() const override {
            return group;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    set == ((const SpvDescriptorGroup*)(other))->set &&
                    binding == ((const SpvDescriptorGroup*)(other))->binding;
        }
    };

    enum class SpvTypename {
        IMAGE, ARRAY, VECTOR, MATRIX, STRUCT
    };

    enum class SpvImageTypename {
        TEX1D, TEX2D, TEX3D, TEX_CUBE, TEX_UNKNOWN //, TEX_BUFFER, TEX_RECT. Note these are not handled yet
    };

    struct SpvType {
        QString name;
        size_t size; // byte size

        virtual ~SpvType() { }
        virtual SpvTypename Type() const = 0;
        virtual bool operator==(const SpvType* other) const = 0;
    };

    struct SpvImageType : public SpvType {
        SpvImageTypename imageTypename;
        bool multisampled;
        bool isDepth;
        bool isArrayed;
        bool sampled; // true is sampler/texture, false is image load/store
        VkFormat format; // Only matters for image load/store

        SpvTypename Type() const override {
            return SpvTypename::IMAGE;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    multisampled == ((const SpvImageType*)(other))->multisampled &&
                    isDepth == ((const SpvImageType*)(other))->isDepth &&
                    isArrayed == ((const SpvImageType*)(other))->isArrayed &&
                    sampled == ((const SpvImageType*)(other))->sampled;
        }
    };

    struct SpvArrayType : public SpvType {
        QVector<size_t> lengths; // num elements
        SpvType* subtype;

        ~SpvArrayType() override {
            if (subtype) delete subtype;
        }
        SpvTypename Type() const override {
            return SpvTypename::ARRAY;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    lengths == ((const SpvArrayType*)(other))->lengths &&
                    subtype == ((const SpvArrayType*)(other))->subtype;
        }
    };

    struct SpvVectorType : public SpvType {
        size_t length; // number of elements

        SpvTypename Type() const override {
            return SpvTypename::VECTOR;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    length == ((const SpvVectorType*)(other))->length;
        }
    };

    struct SpvMatrixType : public SpvType {
        size_t rows;
        size_t columns;

        SpvTypename Type() const override {
            return SpvTypename::MATRIX;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    rows == ((const SpvMatrixType*)(other))->rows &&
                    columns == ((const SpvMatrixType*)(other))->columns;
        }
    };

    struct SpvStructType : public SpvType {
        QVector<SpvType*> members;
        QVector<size_t> memberOffsets;

        ~SpvStructType() override {
            for (auto& member : members) {
                if (member) delete member;
            }
        }
        SpvTypename Type() const override {
            return SpvTypename::STRUCT;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    members == ((const SpvStructType*)(other))->members;
        }
    };

    struct SpvResource {
        QString name;
        SpvGroup* group;
        SpvType* type;

        SpvResource() : name(""), group(nullptr), type(nullptr) { }
        ~SpvResource() {
            if (group) delete group;
            if (type) delete type;
        }
        bool operator==(const SpvResource& other) {
            return name == other.name &&
                    group == other.group &&
                    type == other.type;
        }
        bool operator!=(const SpvResource& other) {
            return !operator==(other);
        }
    };

    using DescriptorLayoutMap = QHash<QPair<uint32_t, uint32_t>, SpvResource*>;
}

#endif // SPIRVRESOURCE_H
