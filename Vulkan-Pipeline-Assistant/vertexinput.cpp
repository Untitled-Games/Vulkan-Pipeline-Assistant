#include "vertexinput.h"

#include <Lib/spirv-cross/spirv_cross.hpp>
#include <QCoreApplication>
#include <QVulkanDeviceFunctions>
#include <QMap>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace vpa;
using namespace SPIRV_CROSS_NAMESPACE;

VertexInput::VertexInput(QVulkanWindow* window, QVulkanDeviceFunctions* deviceFuncs, MemoryAllocator* allocator,
                         QVector<SpirvResource> inputResources, QString meshName, bool isIndexed)
     : m_deviceFuncs(deviceFuncs), m_allocator(allocator), m_indexed(isIndexed), m_indexCount(0) {
    CalculateData(inputResources);
    LoadMesh(meshName, SupportedFormats::OBJ);
}

VertexInput::~VertexInput() {
    m_allocator->Deallocate(m_vertexAllocation);
    if (m_indexed) {
        m_allocator->Deallocate(m_indexAllocation);
    }
}

void VertexInput::BindBuffers(VkCommandBuffer& cmdBuffer) {
    VkDeviceSize offset = 0;
    m_deviceFuncs->vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertexAllocation.buffer, &offset);
    if (m_indexed) {
        m_deviceFuncs->vkCmdBindIndexBuffer(cmdBuffer, m_indexAllocation.buffer, offset, VK_INDEX_TYPE_UINT32);
    }
}

void VertexInput::LoadMesh(QString& meshName, SupportedFormats format) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    std::string warn;
    // TODO change loading based on format
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (meshName + ".obj").toLatin1().data());
    if (!warn.empty()) qWarning("%s", warn.c_str());
    if (!err.empty())  qWarning("%s", err.c_str());

    QVector<uint32_t> indices;
    QVector<float> verts;
    uint32_t count = 0;
    srand(time(NULL));

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            for (int i = 0; i < m_attributes.size(); ++i) {
                if (m_attributes[i] == VertexAttribute::POSITION) {
                    verts.push_back(attrib.vertices[3 * size_t(index.vertex_index) + 0]);
                    verts.push_back(attrib.vertices[3 * size_t(index.vertex_index) + 1]);
                    verts.push_back(attrib.vertices[3 * size_t(index.vertex_index) + 2]);
                }
                else if (attrib.texcoords.size() > 0 && m_attributes[i] == VertexAttribute::TEX_COORD) {
                    verts.push_back(attrib.texcoords[2 * size_t(index.texcoord_index) + 0]);
                    verts.push_back(1.0f - attrib.texcoords[2 * size_t(index.texcoord_index) + 1]);
                }
                else if (attrib.normals.size() > 0 && m_attributes[i] == VertexAttribute::NORMAL) {
                    verts.push_back(attrib.normals[3 * size_t(index.normal_index) + 0]);
                    verts.push_back(attrib.normals[3 * size_t(index.normal_index) + 1]);
                    verts.push_back(attrib.normals[3 * size_t(index.normal_index) + 2]);
                }
                else if (m_attributes[i] == VertexAttribute::RGBA_COLOUR) {
                    verts.push_back(rand() / (float)RAND_MAX);
                    verts.push_back(rand() / (float)RAND_MAX);
                    verts.push_back(rand() / (float)RAND_MAX);
                    verts.push_back(1.0f);
                }
            }
            indices.push_back(count++);
        }
    }

    m_vertexAllocation = m_allocator->Allocate(verts.size() * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Vertex buffer");
    unsigned char* data = m_allocator->MapMemory(m_vertexAllocation);
    memcpy(data, verts.data(), verts.size() * sizeof(float));
    m_allocator->UnmapMemory(m_vertexAllocation);

    if (m_indexed) {
        m_indexCount = indices.size();
        m_indexAllocation = m_allocator->Allocate(m_indexCount * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "Index buffer");
        data = m_allocator->MapMemory(m_indexAllocation);
        memcpy(data, indices.data(), indices.size() * sizeof(uint32_t));
        m_allocator->UnmapMemory(m_indexAllocation);
    }

    qDebug("Loaded mesh %s.obj", qPrintable(meshName));
}

void VertexInput::CalculateData(QVector<SpirvResource>& inputResources) {
    bool usedPos = false;
    QMap<uint32_t, VertexAttribute> attribData;
    for (int i = 0; i < inputResources.size(); ++i) {
        // TODO allow different attrib sizes than float
        auto& res = inputResources[i];
        uint32_t location = res.compiler->get_decoration(res.spirvResource->id, spv::DecorationLocation);
        auto spvType = res.compiler->get_type(res.spirvResource->base_type_id);
        if (spvType.vecsize == 2) {
            attribData[location] = VertexAttribute::TEX_COORD;
        }
        else if (spvType.vecsize == 3 && !usedPos) {
            attribData[location] = VertexAttribute::POSITION;
            usedPos = true;
        }
        else if (spvType.vecsize == 3) {
            attribData[location] = VertexAttribute::NORMAL;
        }
        else if (spvType.vecsize == 4) {
            attribData[location] = VertexAttribute::RGBA_COLOUR;
        }
    }

    m_attributes.resize(inputResources.size());
    int i = 0;
    for (auto attrib : attribData) {
        m_attributes[i++] = attrib;
    }
}

VkVertexInputBindingDescription VertexInput::InputBindingDescription() {
    VkVertexInputBindingDescription desc;
    desc.binding = 0;
    desc.stride = CalculateStride();
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}

QVector<VkVertexInputAttributeDescription> VertexInput::InputAttribDescription() {
    QVector<VkVertexInputAttributeDescription> attribDescs(m_attributes.size());
    uint32_t location = 0;
    uint32_t offset = 0;
    for (int i = 0; i < m_attributes.size(); ++i) {
        VkFormat format;
        uint32_t tmpOffset;
        if (m_attributes[i] == VertexAttribute::TEX_COORD) {
            format = VK_FORMAT_R32G32_SFLOAT;
            tmpOffset = 2 * sizeof(float);
        }
        else if (m_attributes[i] == VertexAttribute::POSITION || m_attributes[i] == VertexAttribute::NORMAL) {
            format = VK_FORMAT_R32G32B32_SFLOAT;
            tmpOffset = 3 * sizeof(float);
        }
        else {
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            tmpOffset = 4 * sizeof(float);
        }
        attribDescs[i].format = format;
        attribDescs[i].offset = offset;
        attribDescs[i].binding = 0;
        attribDescs[i].location = location++;
        offset += tmpOffset;
    }
    return attribDescs;
}

uint32_t VertexInput::CalculateStride() {
    uint32_t stride = 0;
    for (int i = 0; i < m_attributes.size(); ++i) {
        if (m_attributes[i] == VertexAttribute::TEX_COORD) {
            stride += 2 * sizeof(float);
        }
        else if (m_attributes[i] == VertexAttribute::POSITION || m_attributes[i] == VertexAttribute::NORMAL) {
            stride += 3 * sizeof(float);
        }
        else {
            stride += 4 * sizeof(float);
        }
    }
    return stride;
}
