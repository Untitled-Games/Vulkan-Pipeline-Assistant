/*
 * Author: Ralph Ridley
 * Date: 24/12/19
 * Calculate vertex attributes from configuration and prepare vertex and index buffer.
 */
#ifndef VERTEXINPUT_H
#define VERTEXINPUT_H

#include <QVulkanDeviceFunctions>
#include <QString>
#include <Lib/spirv-cross/spirv_cross.hpp>

namespace vpa {
    enum class VertexAttributes {
        POSITION, TEX_COORD, NORMAL, COLOUR
    };

    struct Vec2 {
        float x, y;
    };

    struct Vec3 {
        float x, y, z;
    };

    struct Vec4 {
        float x, y, z, w;
    };

    class VertexInput {
    public:
        VertexInput(QVulkanDeviceFunctions* deviceFuncs, SPIRV_CROSS_NAMESPACE::SmallVector<SPIRV_CROSS_NAMESPACE::Resource>& inputResources, QString meshName, bool isIndexed);
        ~VertexInput();

    private:
        void CalculateData(SPIRV_CROSS_NAMESPACE::SmallVector<SPIRV_CROSS_NAMESPACE::Resource>& inputResources);
        QVulkanDeviceFunctions* m_deviceFuncs;
        VkBuffer m_vertexBuffer;
        VkBuffer m_indexBuffer;
    };
}

#endif // VERTEXINPUT_H
