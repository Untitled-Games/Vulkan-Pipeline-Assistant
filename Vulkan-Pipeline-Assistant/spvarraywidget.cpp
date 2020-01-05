#include "spvarraywidget.h"

#include <QLayout>
#include <QLabel>
#include <QLineEdit>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvstructwidget.h"
#include "spirvresource.h"

using namespace vpa;

SpvArrayWidget::SpvArrayWidget(SpvArrayType* type, QWidget* parent)
    : SpvWidget(parent), m_type(type),  m_data(nullptr), m_totalNumElements(0), m_activeWidgetIdx(0) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);

    m_infoGroup = new QWidget(parent);
    m_infoGroup->setLayout(new QHBoxLayout(m_infoGroup));
    m_infoGroup->layout()->setAlignment(Qt::AlignTop);

    QString infoStr;
    infoStr.append(QStringLiteral("Array(%1) %2D ").arg(SpvTypenameStrings[size_t(m_type->subtype->Type())]).arg(m_type->lengths.size()));
    for (int i = 0; i < m_type->lengths.size(); ++i) {
        infoStr.append(QStringLiteral("[%1").arg(m_type->lengths[i]));
        if (m_type->lengthsUnsized[i]) infoStr.append("*");
        infoStr.append("]");

        m_totalNumElements += m_type->lengths[i];
    }
    m_infoGroup->layout()->addWidget(new QLabel(infoStr, parent));

    m_indicesGroup = new QWidget(parent);
    m_indicesGroup->setLayout(new QHBoxLayout(m_indicesGroup));
    m_indicesGroup->layout()->setAlignment(Qt::AlignTop);
    m_dimensionIndices.resize(m_type->lengths.size());
    for (int i = 0; i < m_dimensionIndices.size(); ++i) {
        QLineEdit* indexEdit = new QLineEdit("0", parent);
        m_dimensionIndices[i] = indexEdit;
        indexEdit->setMaximumWidth(40);
        m_indicesGroup->layout()->addWidget(m_dimensionIndices[i]);
        QObject::connect(indexEdit, &QLineEdit::textChanged, [this, i, indexEdit]() {
            if (indexEdit->text().toUInt() >= m_type->lengths[i]) {
                indexEdit->setText(QString::number(m_type->lengths[i]- 1));
            }
            HandleArrayElementChange();
        });
    }

    m_data = new unsigned char[m_type->size];
    memset(m_data, 0, m_type->size);
    m_stride = m_type->subtype->size;

    if (m_type->subtype->Type() == SpvTypeName::VECTOR) {
        m_inputWidget = new SpvVectorWidget((SpvVectorType*)m_type->subtype, parent);
    }
    else if (m_type->subtype->Type() == SpvTypeName::MATRIX) {
        m_inputWidget = new SpvMatrixWidget((SpvMatrixType*)m_type->subtype, parent);
    }
    else if (m_type->subtype->Type() == SpvTypeName::STRUCT) {
        m_inputWidget = new SpvStructWidget((SpvStructType*)m_type->subtype, parent);
    }
    else {
        qWarning("Invalid type found in spv array.");
    }

    layout->addWidget(m_infoGroup);
    layout->addWidget(m_indicesGroup);
    layout->addWidget(m_inputWidget);
    setLayout(layout);
    HandleArrayElementChange();
}

SpvArrayWidget::~SpvArrayWidget() {
    if (m_data) delete[] m_data;
}

void SpvArrayWidget::Data(unsigned char* dataPtr) const {
    memcpy(dataPtr, m_data, m_type->size);
}

void SpvArrayWidget::Fill(unsigned char* data) {
    memcpy(m_data, data, m_type->size);
}

void SpvArrayWidget::HandleArrayElementChange() {
    int newIndex = 0;
    size_t lengthMult = 1;
    for (int i = m_dimensionIndices.size() - 1; i >= 0; --i) {
        size_t dimIndex = m_dimensionIndices[i]->text().toUInt();
        size_t length = m_type->lengths[i];
        if (dimIndex >= length) {
            qWarning("Index for array dimension %i out of range.", (i+1));
            return;
        }
        else {
            newIndex += dimIndex * lengthMult;
            lengthMult *= length;
        }
    }
    newIndex *= m_stride;

    m_inputWidget->Data(&m_data[m_activeWidgetIdx]);
    m_activeWidgetIdx = newIndex;
    m_inputWidget->Fill(&m_data[m_activeWidgetIdx]);
}
