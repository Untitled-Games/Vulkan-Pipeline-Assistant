#include "spvresourcewidget.h"

#include <QLayout>
#include <QLabel>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvarraywidget.h"
#include "spvstructwidget.h"
#include "spvimagewidget.h"
#include "../Vulkan/spirvresource.h"
#include "../Vulkan/descriptors.h"

namespace vpa {
    SpvResourceWidget::SpvResourceWidget(ContainerWidget* cont, Descriptors* descriptors, SpvResource* resource, uint32_t set, int index, QWidget* parent)
        : SpvWidget(cont, parent), m_resource(resource), m_typeWidget(nullptr), m_descriptors(descriptors), m_set(set), m_index(index) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);
        if (resource->group->Group() != SpvGroupName::PushConstant) {
            m_groupWidget = new QLabel("layout(set = " + QString::number(m_set) + ", binding = " + QString::number(reinterpret_cast<SpvDescriptorGroup*>(resource->group)->binding) + ") " +
                                       SpvGroupNameStrings[size_t(m_resource->group->Group())] + " " + m_resource->name, parent);
        }
        else {
            m_groupWidget = new QLabel(ShaderStageStrings[size_t(reinterpret_cast<SpvPushConstantGroup*>(resource->group)->stage)] + " " + SpvGroupNameStrings[size_t(m_resource->group->Group())] + ", " + m_resource->name, parent);
        }

        m_typeWidget = MakeSpvWidget(m_resource->type, m_container);

        if (m_resource->group->Group() == SpvGroupName::PushConstant) {
            m_typeWidget->Data(m_descriptors->PushConstantData(reinterpret_cast<SpvPushConstantGroup*>(m_resource->group)->stage));
        }
        else {
            m_typeWidget->Data(m_descriptors->MapBufferPointer(m_set, m_index));
            m_descriptors->UnmapBufferPointer(m_set, m_index);
        }

        layout->addWidget(m_groupWidget);
        if (m_resource->type->Type() == SpvTypeName::Image) {
            layout->addWidget(m_typeWidget);
        }
        setLayout(layout);

        hide();
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

    void SpvResourceWidget::OnDrawerInit() {
        if (m_resource->type->Type() == SpvTypeName::Struct) {
            m_typeWidget->SetDrawerItemWidget(GetDrawerItemWidget());
            m_typeWidget->OnDrawerInit();
        }
    }

    void SpvResourceWidget::OnClick(bool expanding) {
        if (expanding) {
            m_container->ShowWidget(this);
            if (m_resource->type->Type() == SpvTypeName::Image) m_typeWidget->show();
        }
        else {
            if (m_resource->type->Type() == SpvTypeName::Image) m_typeWidget->hide();
        }
    }

    bool SpvResourceWidget::event(QEvent* event) {
        if (event->type() == SpvGuiImageChangeType) {
            if (m_typeWidget == nullptr) return true;
            if (m_resource->group->Group() == SpvGroupName::Image) {
                m_descriptors->LoadImage(m_set, m_index, reinterpret_cast<ImageChangeEvent*>(event)->FileName());
            }
        }
        else if (event->type() == SpvGuiDataChangeType) {
            if (m_typeWidget == nullptr) return true;
            if (m_resource->group->Group() == SpvGroupName::PushConstant) {
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
