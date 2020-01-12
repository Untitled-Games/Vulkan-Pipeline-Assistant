#ifndef SPIRVRESOURCE_H
#define SPIRVRESOURCE_H

#include <vulkan/vulkan.h>
#include <QString>
#include <QPair>
#include <QVector>
#include <QDebug>

#include "../common.h"

namespace vpa {
    enum class ShaderStage {
        Vertex, Fragment, TessellationControl, TessellationEvaluation, Geometry, Count_
    };

    extern const QString ShaderStageStrings[size_t(ShaderStage::Count_)];

    inline VkShaderStageFlagBits StageToVkStageFlag(ShaderStage stage) {
        switch (stage) {
        case ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::TessellationControl:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderStage::TessellationEvaluation:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderStage::Count_:
            assert(false);
        }
        return VkShaderStageFlagBits(0);
    }

    enum class SpvGroupName {
        InputAttribute, UniformBuffer, StorageBuffer, PushConstant, Image, Count_
    };

    extern const QString SpvGroupNameStrings[size_t(SpvGroupName::Count_)];

    struct SpvGroup {
        virtual ~SpvGroup() { }
        virtual SpvGroupName Group() const = 0;
        virtual bool operator==(const SpvGroup* other) const = 0;
        virtual QDebug& Print(QDebug& stream) const = 0;
    };

    struct SpvInputAttribGroup : public SpvGroup {
        uint32_t location;

        SpvInputAttribGroup(uint32_t loc) : location(loc) { }
        SpvGroupName Group() const override {
            return SpvGroupName::InputAttribute;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    location == (reinterpret_cast<const SpvInputAttribGroup*>(other))->location;
        }
        QDebug& Print(QDebug& stream) const override {
            return stream << "location " << location;
        }
    };

    struct SpvPushConstantGroup : public SpvGroup {
        ShaderStage stage;

        SpvPushConstantGroup(ShaderStage shaderStage) : stage(shaderStage) { }
        SpvGroupName Group() const override {
            return SpvGroupName::PushConstant;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    stage == (reinterpret_cast<const SpvPushConstantGroup*>(other))->stage;
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

        SpvDescriptorGroup(uint32_t setIdx, uint32_t bindingIdx, VkShaderStageFlags stages, SpvGroupName groupName)
            : set(setIdx), binding(bindingIdx), stageFlags(stages), group(groupName) { }
        void AddStageFlag(VkShaderStageFlagBits flag) { stageFlags |= flag; }
        SpvGroupName Group() const override {
            return group;
        }
        bool operator==(const SpvGroup* other) const override {
            return Group() == other->Group() &&
                    set == (reinterpret_cast<const SpvDescriptorGroup*>(other))->set &&
                    binding == (reinterpret_cast<const SpvDescriptorGroup*>(other))->binding;
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
        Image, Array, Vector, Matrix, Struct, Count_
    };

    enum class SpvImageTypeName {
        Tex1D, Tex2D, Tex3D, TexCube, TexUnknown, Count_ //, TEX_BUFFER, TEX_RECT. Note these are not handled yet
    };

    extern const QString SpvTypeNameStrings[size_t(SpvTypeName::Count_)];
    extern const QString SpvImageTypeNameStrings[size_t(SpvImageTypeName::Count_)];

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
        bool isArrayed;
        bool sampled; // true is sampler/texture, false is image load/store
        VkFormat format; // Only matters for image load/store

        SpvTypeName Type() const override {
            return SpvTypeName::Image;
        }
        bool operator==(const SpvType* other) const override {
            const SpvImageType* otherImage = reinterpret_cast<const SpvImageType*>(other);
            return Type() == other->Type() &&
                    multisampled == otherImage->multisampled && isDepth == otherImage->isDepth &&
                    isArrayed == otherImage->isArrayed && sampled == otherImage->sampled;
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
            return SpvTypeName::Array;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    lengths == (reinterpret_cast<const SpvArrayType*>(other))->lengths &&
                    subtype == (reinterpret_cast<const SpvArrayType*>(other))->subtype;
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
            return SpvTypeName::Vector;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    length == (reinterpret_cast<const SpvVectorType*>(other))->length;
        }
        QDebug& Print(QDebug& stream) const override {
            return stream << "length " << length;
        }
    };

    struct SpvMatrixType : public SpvType {
        size_t rows;
        size_t columns;

        SpvTypeName Type() const override {
            return SpvTypeName::Matrix;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    rows == (reinterpret_cast<const SpvMatrixType*>(other))->rows &&
                    columns == (reinterpret_cast<const SpvMatrixType*>(other))->columns;
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
            return SpvTypeName::Struct;
        }
        bool operator==(const SpvType* other) const override {
            return Type() == other->Type() &&
                    members == (reinterpret_cast<const SpvStructType*>(other))->members;
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
