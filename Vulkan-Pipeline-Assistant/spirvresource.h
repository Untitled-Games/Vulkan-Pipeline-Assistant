#ifndef SPIRVRESOURCE_H
#define SPIRVRESOURCE_H

#include <vulkan/vulkan.h>
#include <QString>
#include <QPair>
#include <QVector>
#include <QDebug>

namespace vpa {
    enum class ShaderStage {
        VERTEX, FRAGMENT, TESS_CONTROL, TESS_EVAL, GEOMETRY, count_
    };

    extern const QString ShaderStageStrings[size_t(ShaderStage::count_)];

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

    enum class SpvGroupName {
        INPUT_ATTRIBUTE, UNIFORM_BUFFER, STORAGE_BUFFER, PUSH_CONSTANT, IMAGE, count_
    };

    extern const QString SpvGroupNameStrings[size_t(SpvGroupName::count_)];

    struct SpvGroup {
        virtual ~SpvGroup() { }
        virtual SpvGroupName Group() const = 0;
        virtual bool operator==(const SpvGroup* other) const = 0;
        virtual QDebug& Print(QDebug& stream) const = 0;
    };

    struct SpvInputAttribGroup : public SpvGroup {
        uint32_t location;

        SpvInputAttribGroup(uint32_t location) : location(location) { }
        SpvGroupName Group() const override {
            return SpvGroupName::INPUT_ATTRIBUTE;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    location == ((const SpvInputAttribGroup*)(other))->location;
        }
        QDebug& Print(QDebug& stream) const override {
            return stream << "location " << location;
        }
    };

    struct SpvPushConstantGroup : public SpvGroup {
        ShaderStage stage;

        SpvPushConstantGroup(ShaderStage stage) : stage(stage) { }
        SpvGroupName Group() const override {
            return SpvGroupName::PUSH_CONSTANT;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    stage == ((const SpvPushConstantGroup*)(other))->stage;
        }
        QDebug& Print(QDebug& stream) const override {
            return stream << "stage " << ShaderStageStrings[size_t(stage)];
        }
    };

    struct SpvDescriptorGroup : public SpvGroup {
        uint32_t set;
        uint32_t binding;
        VkShaderStageFlags stageFlags; // Can have multiple stages, so compress to a flag
        SpvGroupName group;

        SpvDescriptorGroup(uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags, SpvGroupName group)
            : set(set), binding(binding), stageFlags(stageFlags), group(group) { }
        void AddStageFlag(VkShaderStageFlagBits flag) { stageFlags |= flag; }
        SpvGroupName Group() const override {
            return group;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    set == ((const SpvDescriptorGroup*)(other))->set &&
                    binding == ((const SpvDescriptorGroup*)(other))->binding;
        }
        QDebug& Print(QDebug& stream) const override {
            return stream << "set " << set << " binding " << binding << " stage flags " << stageFlags;
        }
    };
    inline QDebug operator<<(QDebug stream, const SpvGroup* group) {
        stream << "SpvGroup " << SpvGroupNameStrings[size_t(group->Group())] << " ";
        group->Print(stream);
        return stream;
    }

    enum class SpvTypeName {
        IMAGE, ARRAY, VECTOR, MATRIX, STRUCT, count_
    };

    enum class SpvImageTypeName {
        TEX1D, TEX2D, TEX3D, TEX_CUBE, TEX_UNKNOWN, count_ //, TEX_BUFFER, TEX_RECT. Note these are not handled yet
    };

    extern const QString SpvTypeNameStrings[size_t(SpvTypeName::count_)];
    extern const QString SpvImageTypeNameStrings[size_t(SpvImageTypeName::count_)];

    struct SpvType {
        QString name;
        size_t size; // byte size

        virtual ~SpvType() { }
        virtual SpvTypeName Type() const = 0;
        virtual bool operator==(const SpvType* other) const = 0;
        virtual QDebug& Print(QDebug& stream) const = 0;
    };

    struct SpvImageType : public SpvType {
        SpvImageTypeName imageTypename;
        bool multisampled;
        bool isDepth;
        bool isArrayed; // TODO deal with arrayed images
        bool sampled; // true is sampler/texture, false is image load/store
        VkFormat format; // Only matters for image load/store

        SpvTypeName Type() const override {
            return SpvTypeName::IMAGE;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    multisampled == ((const SpvImageType*)(other))->multisampled &&
                    isDepth == ((const SpvImageType*)(other))->isDepth &&
                    isArrayed == ((const SpvImageType*)(other))->isArrayed &&
                    sampled == ((const SpvImageType*)(other))->sampled;
        }
        QDebug& Print(QDebug& stream) const override {
            return stream << "Image Type " << SpvImageTypeNameStrings[size_t(imageTypename)] <<
                             " ms " << multisampled << " depth " << isDepth << " arrayed " << isArrayed <<
                             " sampled " << sampled << " VkFormat " << format;
        }
    };

    struct SpvArrayType : public SpvType {
        QVector<size_t> lengths; // num elements
        QVector<bool> lengthsUnsized; // unsized = true, otherwise false
        SpvType* subtype;

        ~SpvArrayType() override {
            if (subtype) delete subtype;
        }
        SpvTypeName Type() const override {
            return SpvTypeName::ARRAY;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    lengths == ((const SpvArrayType*)(other))->lengths &&
                    subtype == ((const SpvArrayType*)(other))->subtype;
        }
        QDebug& Print(QDebug& stream) const override {
            stream << "Dimensions " << lengths.size();
            for (int i = 0; i < lengths.size(); ++i) {
                stream << " [" << lengths[i] << "]" << " unsized " << lengthsUnsized[i];
            }
            stream << "\n    subtype " << SpvTypeNameStrings[size_t(subtype->Type())] << " ";
            subtype->Print(stream);
            return stream;
        }
    };

    struct SpvVectorType : public SpvType {
        size_t length; // number of elements

        SpvTypeName Type() const override {
            return SpvTypeName::VECTOR;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    length == ((const SpvVectorType*)(other))->length;
        }
        QDebug& Print(QDebug& stream) const override {
            return stream << "length " << length;
        }
    };

    struct SpvMatrixType : public SpvType {
        size_t rows;
        size_t columns;

        SpvTypeName Type() const override {
            return SpvTypeName::MATRIX;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    rows == ((const SpvMatrixType*)(other))->rows &&
                    columns == ((const SpvMatrixType*)(other))->columns;
        }
        QDebug& Print(QDebug& stream) const override {
            return stream << "rows " << rows << " columns " << columns;
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
        SpvTypeName Type() const override {
            return SpvTypeName::STRUCT;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    members == ((const SpvStructType*)(other))->members;
        }
        QDebug& Print(QDebug& stream) const override {
            for (int i = 0; i < members.size(); ++i) {
                stream << "\n   Member " << i << " offset " << memberOffsets[i] << " " <<
                          SpvTypeNameStrings[size_t(members[i]->Type())] << " ";
                members[i]->Print(stream);
            }
            return stream;
        }
    };

    inline QDebug operator<<(QDebug stream, const SpvType* type) {
        stream << "SpvType " << SpvTypeNameStrings[size_t(type->Type())] << " ";
        type->Print(stream);
        return stream;
    }

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

    inline QDebug operator<<(QDebug stream, const SpvResource* res) {
        return stream << "SpvResource " << res->name << "\n" <<
                  "   " << res->group << "\n" <<
                  "   " << res->type;
    }

    using DescriptorLayoutMap = QHash<QPair<uint32_t, uint32_t>, SpvResource*>;
}

#endif // SPIRVRESOURCE_H
