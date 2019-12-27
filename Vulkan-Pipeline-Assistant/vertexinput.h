#ifndef VERTEXINPUT_H
#define VERTEXINPUT_H

#include "spirvresource.h"
#include "memoryallocator.h"

#include <QVector>

class QVulkanDeviceFunctions;
class QVulkanWindow;
namespace vpa {
    enum class VertexAttribute {
        POSITION,
        TEX_COORD,
        NORMAL,
        RGBA_COLOUR,
        count_
    };

    enum class SupportedFormats {
        OBJ,
        count_
    };

    class VertexInput {
    public:
        VertexInput(QVulkanWindow* window, QVulkanDeviceFunctions* deviceFuncs, QVector<SpirvResource>& inputResources, QString meshName, bool isIndexed);
        ~VertexInput();

        VkBuffer VertexBuffer() {
            return m_vertexAllocation.buffer;
        }

        VkBuffer IndexBuffer() {
            return m_indexAllocation.buffer;
        }

        void BindBuffers(VkCommandBuffer& cmdBuffer);

        VkVertexInputBindingDescription InputBindingDescription();
        QVector<VkVertexInputAttributeDescription> InputAttribDescription();

    private:
        void AssignDefaultMeaning(QVector<SpirvResource>& inputResources);
        void LoadMesh(QString& meshName, SupportedFormats format);
        void CalculateData(QVector<SpirvResource>& inputResources);
        uint32_t CalculateStride();

        bool m_indexed;
        QVulkanDeviceFunctions* m_deviceFuncs;
        QVector<VertexAttribute> m_attributes; // mapped by input location
        Allocation m_vertexAllocation;
        Allocation m_indexAllocation;
        MemoryAllocator m_allocator;
    };
}

#endif // VERTEXINPUT_H
