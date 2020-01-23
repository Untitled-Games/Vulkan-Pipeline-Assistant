#include "vertexinput.h"

#include <time.h>
#include <QCoreApplication>
#include <QVulkanDeviceFunctions>
#include <QMap>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace vpa {
    VertexInput::VertexInput(QVulkanDeviceFunctions* deviceFuncs, MemoryAllocator* allocator,
                             QVector<SpvResource*> inputResources, QString meshName, bool isIndexed, VPAError& err)
        : m_indexed(isIndexed), m_indexCount(0), m_deviceFuncs(deviceFuncs), m_vertexAllocation({}), m_indexAllocation({}), m_allocator(allocator) {
        CalculateData(inputResources);
        err = LoadMesh(meshName, SupportedFormats::Obj);
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

    VPAError VertexInput::LoadMesh(QString& meshName, SupportedFormats format) {
        if (format == SupportedFormats::Obj) {
            return LoadObj(meshName);
        }
        return VPA_WARN("Unsupported format for loading mesh");
    }

    VPAError VertexInput::LoadObj(QString& meshName) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string objErr;
        std::string warn;
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &objErr, (meshName + ".obj").toLatin1().data())) {
            if (!objErr.empty()) return VPA_WARN(QString::fromStdString(objErr));
            return VPA_WARN("Obj mesh loading failed.");
        }
        if (!warn.empty()) qDebug(warn.c_str());

        QVector<uint32_t> indices;
        QVector<float> verts;
        uint32_t count = 0;
        srand(static_cast<unsigned int>(time(NULL)));

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                for (int i = 0; i < m_attributes.size(); ++i) {
                    if (m_attributes[i] == VertexAttribute::Position) {
                        verts.push_back(attrib.vertices[3 * size_t(index.vertex_index) + 0]);
                        verts.push_back(attrib.vertices[3 * size_t(index.vertex_index) + 1]);
                        verts.push_back(attrib.vertices[3 * size_t(index.vertex_index) + 2]);
                    }
                    else if (attrib.texcoords.size() > 0 && m_attributes[i] == VertexAttribute::TexCoord) {
                        verts.push_back(attrib.texcoords[2 * size_t(index.texcoord_index) + 0]);
                        verts.push_back(1.0f - attrib.texcoords[2 * size_t(index.texcoord_index) + 1]);
                    }
                    else if (attrib.normals.size() > 0 && m_attributes[i] == VertexAttribute::Normal) {
                        verts.push_back(attrib.normals[3 * size_t(index.normal_index) + 0]);
                        verts.push_back(attrib.normals[3 * size_t(index.normal_index) + 1]);
                        verts.push_back(attrib.normals[3 * size_t(index.normal_index) + 2]);
                    }
                    else if (m_attributes[i] == VertexAttribute::RgbaColour) {
                        verts.push_back(rand() / float(RAND_MAX));
                        verts.push_back(rand() / float(RAND_MAX));
                        verts.push_back(rand() / float(RAND_MAX));
                        verts.push_back(1.0f);
                    }
                }
                indices.push_back(count++);
            }
        }

        VPA_PASS_ERROR(m_allocator->Allocate(size_t(verts.size()) * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "Vertex buffer", m_vertexAllocation))
        unsigned char* data = m_allocator->MapMemory(m_vertexAllocation);
        memcpy(data, verts.data(), size_t(verts.size()) * sizeof(float));
        m_allocator->UnmapMemory(m_vertexAllocation);

        if (m_indexed) {
            m_indexCount = uint32_t(indices.size());
            VPA_PASS_ERROR(m_allocator->Allocate(m_indexCount * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "Index buffer", m_indexAllocation))
            data = m_allocator->MapMemory(m_indexAllocation);
            memcpy(data, indices.data(), size_t(indices.size()) * sizeof(uint32_t));
            m_allocator->UnmapMemory(m_indexAllocation);
        }

        qDebug("Loaded mesh %s.obj", qPrintable(meshName));
        return VPA_OK;
    }

    void VertexInput::CalculateData(QVector<SpvResource*>& inputResources) {
        bool usedPos = false;
        QMap<uint32_t, VertexAttribute> attribData;
        for (int i = 0; i < inputResources.size(); ++i) {
            // TODO allow different attrib sizes than float
            uint32_t location = reinterpret_cast<SpvInputAttribGroup*>(inputResources[i]->group)->location;
            SpvVectorType* type = reinterpret_cast<SpvVectorType*>(inputResources[i]->type); // TODO deal with other types
            if (type->length == 2) {
                attribData[location] = VertexAttribute::TexCoord;
            }
            else if (type->length == 3 && !usedPos) {
                attribData[location] = VertexAttribute::Position;
                usedPos = true;
            }
            else if (type->length == 3) {
                attribData[location] = VertexAttribute::Normal;
            }
            else if (type->length == 4) {
                attribData[location] = VertexAttribute::RgbaColour;
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
            if (m_attributes[i] == VertexAttribute::TexCoord) {
                format = VK_FORMAT_R32G32_SFLOAT;
                tmpOffset = 2 * sizeof(float);
            }
            else if (m_attributes[i] == VertexAttribute::Position || m_attributes[i] == VertexAttribute::Normal) {
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
            if (m_attributes[i] == VertexAttribute::TexCoord) {
                stride += 2 * sizeof(float);
            }
            else if (m_attributes[i] == VertexAttribute::Position || m_attributes[i] == VertexAttribute::Normal) {
                stride += 3 * sizeof(float);
            }
            else {
                stride += 4 * sizeof(float);
            }
        }
        return stride;
    }
}
