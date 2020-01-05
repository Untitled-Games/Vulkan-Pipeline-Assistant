#include "spvvectorwidget.h"

#include <QLineEdit>
#include <QLabel>
#include <QLayout>

#include "spirvresource.h"

using namespace vpa;

const QString SpvVectorWidget::VecStrings[4] = { "float", "vec2", "vec3", "vec4" };
const QString SpvVectorWidget::ComponentStrings[4] = { "x", "y", "z", "w" };

SpvVectorWidget::SpvVectorWidget(SpvVectorType* type, QWidget* parent)
    : SpvWidget(parent), m_type(type) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);

    m_infoGroup = new QWidget(parent);
    m_infoGroup->setLayout(new QHBoxLayout(m_infoGroup));
    m_infoGroup->layout()->addWidget(new QLabel(VecStrings[(m_type->length - 1)], parent));
    m_infoGroup->layout()->setAlignment(Qt::AlignTop);

    m_inputsGroup = new QWidget(parent);
    QGridLayout* inputsGroupLayout = new QGridLayout(m_inputsGroup);
    for (size_t i = 0; i < m_type->length; ++i) {
        m_inputs[i] = new QLineEdit(m_inputsGroup);
        inputsGroupLayout->addWidget(new QLabel(ComponentStrings[i]), 0, int(i));
        inputsGroupLayout->addWidget(m_inputs[i], 1, int(i));
    }

    m_inputsGroup->setLayout(inputsGroupLayout);
    layout->addWidget(m_infoGroup);
    layout->addWidget(m_inputsGroup);
    setLayout(layout);
}

void SpvVectorWidget::Data(unsigned char* dataPtr) const {
    float* floatPtr = reinterpret_cast<float*>(dataPtr);
    for (size_t i = 0; i < m_type->length; ++i) {
        floatPtr[i] = m_inputs[i]->text().toFloat();
    }
}

void SpvVectorWidget::Fill(unsigned char* data) {
    float* floatPtr = reinterpret_cast<float*>(data);
    for (size_t i = 0; i < m_type->length; ++i) {
        m_inputs[i]->setText(QString::number(floatPtr[i]));
    }
}
