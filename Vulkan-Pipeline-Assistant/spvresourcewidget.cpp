#include "spvresourcewidget.h"

#include <QLayout>
#include <QLabel>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvarraywidget.h"
#include "spvstructwidget.h"
#include "spirvresource.h"

using namespace vpa;

SpvResourceWidget::SpvResourceWidget(SpvResource* resource, QWidget* parent)
    : SpvWidget(parent), m_resource(resource) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    m_groupWidget = new QLabel(SpvGroupnameStrings[size_t(m_resource->group->Group())] + " " + m_resource->name, parent); // TODO more specific info

    if (m_resource->type->Type() == SpvTypeName::VECTOR) {
        m_typeWidget = new SpvVectorWidget((SpvVectorType*)m_resource->type, parent);
    }
    else if (m_resource->type->Type() == SpvTypeName::MATRIX) {
        m_typeWidget = new SpvMatrixWidget((SpvMatrixType*)m_resource->type, parent);
    }
    else if (m_resource->type->Type() == SpvTypeName::STRUCT) {
        m_typeWidget = new SpvStructWidget((SpvStructType*)m_resource->type, parent);
    }
    else if (m_resource->type->Type() == SpvTypeName::ARRAY) {
        m_typeWidget = new SpvArrayWidget((SpvArrayType*)m_resource->type, parent);
    }
    else {
        qWarning("Invalid type found in spv array.");
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
