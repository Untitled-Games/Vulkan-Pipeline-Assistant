#include "spvstructwidget.h"

#include <QPushButton>
#include <QLabel>
#include <QLayout>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvarraywidget.h"
#include "spvstructwidget.h"
#include "spirvresource.h"

namespace vpa {
    SpvStructWidget::SpvStructWidget(SpvStructType* type, QWidget* parent)
        : SpvWidget(parent), m_type(type), m_activeIndex(0) {
        QVBoxLayout* layout = new QVBoxLayout(this);

        m_infoGroup = new QWidget(parent);
        m_infoGroup->setLayout(new QHBoxLayout(m_infoGroup));
        m_infoGroup->layout()->addWidget(new QLabel(QStringLiteral("struct"), parent));
        m_infoGroup->layout()->setAlignment(Qt::AlignTop);

        m_inputsGroup = new QWidget(parent);
        m_buttonsGroup = new QWidget(m_inputsGroup);
        m_typesGroup = new QWidget(m_inputsGroup);
        QHBoxLayout* inputLayout = new QHBoxLayout(m_inputsGroup);
        QVBoxLayout* buttonsLayout = new QVBoxLayout(m_buttonsGroup);
        QVBoxLayout* typesLayout = new QVBoxLayout(m_typesGroup);
        inputLayout->setAlignment(Qt::AlignTop);
        buttonsLayout->setAlignment(Qt::AlignTop);
        typesLayout->setAlignment(Qt::AlignTop);

        inputLayout->addWidget(m_buttonsGroup);
        inputLayout->addWidget(m_typesGroup);
        m_inputsGroup->setLayout(inputLayout);

        m_typeWidgets.resize(m_type->members.size());
        m_memberButtons.resize(m_type->members.size());
        for (int i = 0; i < m_typeWidgets.size(); ++i) {
            if (m_type->members[i]->Type() == SpvTypeName::VECTOR) {
                m_typeWidgets[i] = new SpvVectorWidget(reinterpret_cast<SpvVectorType*>(m_type->members[i]), parent);
            }
            else if (m_type->members[i]->Type() == SpvTypeName::MATRIX) {
                m_typeWidgets[i] = new SpvMatrixWidget(reinterpret_cast<SpvMatrixType*>(m_type->members[i]), parent);
            }
            else if (m_type->members[i]->Type() == SpvTypeName::STRUCT) {
                m_typeWidgets[i] = new SpvStructWidget(reinterpret_cast<SpvStructType*>(m_type->members[i]), parent);
            }
            else if (m_type->members[i]->Type() == SpvTypeName::ARRAY) {
                m_typeWidgets[i] = new SpvArrayWidget(reinterpret_cast<SpvArrayType*>(m_type->members[i]), parent);
            }
            else {
                qWarning("Invalid type found in spv struct.");
            }
            m_typeWidgets[i]->hide();
            m_memberButtons[i] = new QPushButton(m_type->members[i]->name, m_buttonsGroup);
            QObject::connect(m_memberButtons[i], &QPushButton::released, [this, i]{ this->HandleMemberButtonPress(i); });

            buttonsLayout->addWidget(m_memberButtons[i]);
            typesLayout->addWidget(m_typeWidgets[i]);
            m_buttonsGroup->setLayout(buttonsLayout);
        }

        m_typesGroup->setLayout(typesLayout);

        layout->addWidget(m_infoGroup);
        layout->addWidget(m_inputsGroup);
        setLayout(layout);
        HandleMemberButtonPress(0);
    }

    void SpvStructWidget::Data(unsigned char* dataPtr) const {
        size_t offset = 0;
        for (int i = 0; i < m_typeWidgets.size(); ++i) {
            offset = m_type->memberOffsets[i];
            m_typeWidgets[i]->Data(dataPtr + offset);
        }
    }

    void SpvStructWidget::Fill(const unsigned char* data) {
        size_t offset = 0;
        for (int i = 0; i < m_typeWidgets.size(); ++i) {
            offset = m_type->memberOffsets[i];
            m_typeWidgets[i]->Fill(data + offset);
        }
    }

    void SpvStructWidget::HandleMemberButtonPress(int idx) {
        m_typeWidgets[m_activeIndex]->hide();
        m_activeIndex = idx;
        m_typeWidgets[m_activeIndex]->show();
    }
}
