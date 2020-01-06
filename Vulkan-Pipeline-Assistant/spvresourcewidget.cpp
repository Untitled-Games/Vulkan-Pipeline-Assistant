#include "spvresourcewidget.h"

#include <QLayout>
#include <QLabel>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvarraywidget.h"
#include "spvstructwidget.h"
#include "spirvresource.h"
#include "descriptors.h"

using namespace vpa;

SpvResourceWidget::SpvResourceWidget(Descriptors* descriptors, SpvResource* resource, uint32_t set, int index, QWidget* parent)
    : SpvWidget(parent), m_resource(resource), m_typeWidget(nullptr), m_descriptors(descriptors), m_set(set), m_index(index) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    m_groupWidget = new QLabel(SpvGroupnameStrings[size_t(m_resource->group->Group())] + " " + m_resource->name, parent); // TODO more specific info

    if (m_resource->type->Type() == SpvTypeName::VECTOR) {
        m_typeWidget = new SpvVectorWidget((SpvVectorType*)m_resource->type, this);
    }
    else if (m_resource->type->Type() == SpvTypeName::MATRIX) {
        m_typeWidget = new SpvMatrixWidget((SpvMatrixType*)m_resource->type, this);
    }
    else if (m_resource->type->Type() == SpvTypeName::STRUCT) {
        m_typeWidget = new SpvStructWidget((SpvStructType*)m_resource->type, this);
    }
    else if (m_resource->type->Type() == SpvTypeName::ARRAY) {
        m_typeWidget = new SpvArrayWidget((SpvArrayType*)m_resource->type, this);
    }
    else {
        qWarning("Invalid type found in spv array.");
    }

    if (m_resource->group->Group() == SpvGroupName::IMAGE) {

    }
    else if (m_resource->group->Group() == SpvGroupName::PUSH_CONSTANT) {

    }
    else {
        m_typeWidget->Fill(m_descriptors->MapBufferPointer(m_set, m_index));
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

void SpvResourceWidget::Fill(unsigned char* data) {
    m_typeWidget->Fill(data);
}

bool SpvResourceWidget::event(QEvent* event) {
    if (event->type() == SpvGuiDataChangeType) {
        if (m_typeWidget == nullptr) return true;
        if (m_resource->group->Group() == SpvGroupName::IMAGE) {

        }
        else if (m_resource->group->Group() == SpvGroupName::PUSH_CONSTANT) {

        }
        else {
            m_typeWidget->Data(m_descriptors->MapBufferPointer(m_set, m_index));
            m_descriptors->UnmapBufferPointer(m_set, m_index);
        }
        return true;
    }
    return false;
}
