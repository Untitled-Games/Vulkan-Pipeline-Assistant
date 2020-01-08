#include "spvresourcewidget.h"

#include <QLayout>
#include <QLabel>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvarraywidget.h"
#include "spvstructwidget.h"
#include "spvimagewidget.h"
#include "spirvresource.h"
#include "descriptors.h"

namespace vpa {
    SpvResourceWidget::SpvResourceWidget(Descriptors* descriptors, SpvResource* resource, uint32_t set, int index, QWidget* parent)
        : SpvWidget(parent), m_resource(resource), m_typeWidget(nullptr), m_descriptors(descriptors), m_set(set), m_index(index) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);
        m_groupWidget = new QLabel(SpvGroupNameStrings[size_t(m_resource->group->Group())] + " " + m_resource->name, parent); // TODO more specific info

        if (m_resource->type->Type() == SpvTypeName::VECTOR) {
            m_typeWidget = new SpvVectorWidget(reinterpret_cast<SpvVectorType*>(m_resource->type), this);
        }
        else if (m_resource->type->Type() == SpvTypeName::MATRIX) {
            m_typeWidget = new SpvMatrixWidget(reinterpret_cast<SpvMatrixType*>(m_resource->type), this);
        }
        else if (m_resource->type->Type() == SpvTypeName::STRUCT) {
            m_typeWidget = new SpvStructWidget(reinterpret_cast<SpvStructType*>(m_resource->type), this);
        }
        else if (m_resource->type->Type() == SpvTypeName::ARRAY) {
            m_typeWidget = new SpvArrayWidget(reinterpret_cast<SpvArrayType*>(m_resource->type), this);
        }
        else if (m_resource->type->Type() == SpvTypeName::IMAGE) {
            m_typeWidget = new SpvImageWidget(reinterpret_cast<SpvImageType*>(m_resource->type), this);
        }
        else {
            qWarning("Invalid type found in spv array.");
        }

        if (m_resource->group->Group() == SpvGroupName::PUSH_CONSTANT) {
            m_typeWidget->Data(m_descriptors->PushConstantData(reinterpret_cast<SpvPushConstantGroup*>(m_resource->group)->stage));
        }
        else {
            m_typeWidget->Data(m_descriptors->MapBufferPointer(m_set, m_index));
            m_descriptors->UnmapBufferPointer(m_set, m_index);
        }

        layout->addWidget(m_groupWidget);
        layout->addWidget(m_typeWidget);
        setLayout(layout);
    }

    QString SpvResourceWidget::Title() const {
        return m_resource->name;
    }

    void SpvResourceWidget::Data(unsigned char* dataPtr) const {
        m_typeWidget->Data(dataPtr);
    }

    void SpvResourceWidget::Fill(const unsigned char* data) {
        m_typeWidget->Fill(data);
    }

    bool SpvResourceWidget::event(QEvent* event) {
        if (event->type() == SpvGuiImageChangeType) {
            if (m_typeWidget == nullptr) return true;
            if (m_resource->group->Group() == SpvGroupName::IMAGE) {
                m_descriptors->LoadImage(m_set, m_index, reinterpret_cast<ImageChangeEvent*>(event)->FileName());
            }
        }
        else if (event->type() == SpvGuiDataChangeType) {
            if (m_typeWidget == nullptr) return true;
            if (m_resource->group->Group() == SpvGroupName::PUSH_CONSTANT) {
                unsigned char* dataPtr = m_descriptors->PushConstantData(reinterpret_cast<SpvPushConstantGroup*>(m_resource->group)->stage);
                if (dataPtr != nullptr) {
                    m_typeWidget->Data(dataPtr);
                }
            }
            else {
                unsigned char* dataPtr = m_descriptors->MapBufferPointer(m_set, m_index);
                if (dataPtr != nullptr) {
                    m_typeWidget->Data(dataPtr);
                    m_descriptors->UnmapBufferPointer(m_set, m_index);
                }
            }
            return true;
        }
        return false;
    }
}
