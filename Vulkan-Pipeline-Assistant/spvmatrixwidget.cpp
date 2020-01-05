#include "spvmatrixwidget.h"

#include <QLineEdit>
#include <QLabel>
#include <QLayout>

#include "spirvresource.h"

using namespace vpa;

SpvMatrixWidget::SpvMatrixWidget(SpvMatrixType* type, QWidget* parent)
    : SpvWidget(parent), m_type(type) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);

    m_infoGroup = new QWidget(parent);
    m_infoGroup->setLayout(new QHBoxLayout(m_infoGroup));
    m_infoGroup->layout()->setAlignment(Qt::AlignTop);
    m_infoGroup->layout()->addWidget(new QLabel(QStringLiteral("mat %1x%2").arg(m_type->rows).arg(m_type->columns), parent));

    m_inputsGroup = new QWidget(parent);
    QGridLayout* inputsGroupLayout = new QGridLayout(m_inputsGroup);
    inputsGroupLayout->setAlignment(Qt::AlignTop);
    for (size_t row = 0; row < m_type->rows; ++row) {
        for (size_t col = 0; col < m_type->columns; ++col) {
            m_inputs[row][col] = new QLineEdit(m_inputsGroup);
            inputsGroupLayout->addWidget(m_inputs[row][col], int(row), int(col));
        }
    }

    m_inputsGroup->setLayout(inputsGroupLayout);
    layout->addWidget(m_infoGroup);
    layout->addWidget(m_inputsGroup);
    setLayout(layout);
}

void SpvMatrixWidget::Data(unsigned char* dataPtr) const {
    float* floatPtr = reinterpret_cast<float*>(dataPtr);
    for (size_t row = 0; row < m_type->rows; ++row) {
        for (size_t col = 0; col < m_type->columns; ++col) {
            floatPtr[row * m_type->columns + col] = m_inputs[row][col]->text().toFloat();
        }
    }
}

void SpvMatrixWidget::Fill(unsigned char* data) {
    float* floatPtr = reinterpret_cast<float*>(data);
    for (size_t row = 0; row < m_type->rows; ++row) {
        for (size_t col = 0; col < m_type->columns; ++col) {
            m_inputs[row][col]->setText(QString::number(floatPtr[row * m_type->columns + col]));
        }
    }
}
