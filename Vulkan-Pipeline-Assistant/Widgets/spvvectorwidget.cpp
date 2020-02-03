#include "spvvectorwidget.h"

#include <QLineEdit>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QValidator>

#include "../Vulkan/spirvresource.h"
#include "descriptortree.h"
#include "common.h"

namespace vpa {
    const QString SpvVectorWidget::ComponentStrings[4] = { "x", "y", "z", "w" };

    SpvVectorWidget::SpvVectorWidget(SpvVectorType* type, DescriptorNodeRoot* root) : SpvWidget(root), m_type(type) {
        QGridLayout* layout = new QGridLayout(this);
        layout->setAlignment(Qt::AlignTop);

        size_t i;
        for (i = 0; i < m_type->length; ++i) {
            m_inputs[i] = new QLineEdit(this);
            layout->addWidget(new QLabel(ComponentStrings[i]), 0, int(i));
            layout->addWidget(m_inputs[i], 1, int(i));
            QObject::connect(m_inputs[i], &QLineEdit::textChanged, [this] { m_root->WriteDescriptorData(); });
        }

        QPushButton* normBtn = new QPushButton("Normalize", this);
        connect(normBtn, SIGNAL(pressed()), this, SLOT(HandleNormalize()));
        layout->addWidget(normBtn, 0, int(i));

        setLayout(layout);
    }

    void SpvVectorWidget::Data(unsigned char* dataPtr) const {
        float* floatPtr = reinterpret_cast<float*>(dataPtr);
        for (size_t i = 0; i < m_type->length; ++i) {
            floatPtr[i] = m_inputs[i]->text().toFloat();
        }
    }

    void SpvVectorWidget::Fill(const unsigned char* data) {
        const float* floatPtr = reinterpret_cast<const float*>(data);
        for (size_t i = 0; i < m_type->length; ++i) {
            m_inputs[i]->setText(QString::number(double(floatPtr[i])));
        }
    }

    void SpvVectorWidget::InitData() {
        Fill(BYTE_CPTR(DefaultData));
    }

    void SpvVectorWidget::HandleNormalize() {
        float length = 0;
        for (size_t i = 0; i < m_type->length; ++i) {
            float val = m_inputs[i]->text().toFloat();
            length += val * val;
        }
        if (length > 0) {
            length = sqrt(length);
            for (size_t i = 0; i < m_type->length; ++i) {
                float val = m_inputs[i]->text().toFloat();
                m_inputs[i]->setText(QString::number(double(val / length)));
            }
        }
    }
}
