#ifndef VERTEXINPUT_H
#define VERTEXINPUT_H

#include <QVector>

#include "spirvresource.h"
#include "memoryallocator.h"


class QVulkanDeviceFunctions;
class QVulkanWindow;
namespace vpa {
    enum class VertexAttribute {
        Position,
        TexCoord,
        Normal,
        RgbaColour,
        Count_
    };

    enum class SupportedFormats {
        Obj,
        Count_
    };

    class VertexInput final {
    public:
        VertexInput(QVulkanDeviceFunctions* deviceFuncs, MemoryAllocator* allocator,
                    QVector<SpvResource*> inputResources, QString meshName, bool isIndexed, VPAError& err);
        ~VertexInput();

        VkBuffer VertexBuffer() const { return m_vertexAllocation.buffer;  }
        VkBuffer IndexBuffer() const { return m_indexAllocation.buffer; }
        bool IsIndexed() const {  return m_indexed; }
        uint32_t IndexCount() const { return m_indexCount; }

        void BindBuffers(VkCommandBuffer& cmdBuffer);

        VkVertexInputBindingDescription InputBindingDescription();
        QVector<VkVertexInputAttributeDescription> InputAttribDescription();

    private:
        VPAError LoadMesh(QString& meshName, SupportedFormats format);
        VPAError LoadObj(QString& meshName);

        void AssignDefaultMeaning(QVector<SpvResource*>& inputResources);
        void CalculateData(QVector<SpvResource*>& inputResources);
        uint32_t CalculateStride();

        bool m_indexed;
        uint32_t m_indexCount;
        QVulkanDeviceFunctions* m_deviceFuncs;
        QVector<VertexAttribute> m_attributes;
        Allocation m_vertexAllocation;
        Allocation m_indexAllocation;
        MemoryAllocator* m_allocator;
    };
}

#endif // VERTEXINPUT_H
