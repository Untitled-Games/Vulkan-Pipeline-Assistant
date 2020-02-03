#include "spvarraywidget.h"

#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCoreApplication>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvstructwidget.h"
#include "descriptortree.h"
#include "../Vulkan/spirvresource.h"

namespace vpa {
    SpvArrayWidget::SpvArrayWidget(SpvArrayType* type, DescriptorNodeRoot* root, ContainerWidget*& subContainer)
        : SpvWidget(root), m_type(type), m_inputLeaf(nullptr), m_data(nullptr), m_totalNumElements(0), m_activeWidgetIdx(0) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);

        m_inputArea = new ContainerWidget(this);
        subContainer = m_inputArea;

        for (int i = 0; i < m_type->lengths.size(); ++i) {
            m_totalNumElements += m_type->lengths[i];
        }

        m_indicesGroup = new QWidget(this);
        m_indicesGroup->setLayout(new QHBoxLayout(m_indicesGroup));
        m_indicesGroup->layout()->setAlignment(Qt::AlignTop);
        m_dimensionIndices.resize(m_type->lengths.size());
        for (int i = 0; i < m_dimensionIndices.size(); ++i) {
            m_dimensionIndices[i] = 0;
            QSpinBox* indexEdit = new QSpinBox(this);
            indexEdit->setRange(0, int(m_type->lengths[i]) - 1);
            indexEdit->setMaximumWidth(40);
            m_indicesGroup->layout()->addWidget(indexEdit);
            QObject::connect(indexEdit, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){
                m_dimensionIndices[i] = size_t(indexEdit->value());
                HandleArrayElementChange();
            });
        }

        m_data = new unsigned char[m_type->size];
        memset(m_data, 0, m_type->size);
        m_stride = m_type->subtype->size;

        layout->addWidget(m_indicesGroup);
        layout->addWidget(subContainer);
        setLayout(layout);
    }

    SpvArrayWidget::~SpvArrayWidget() {
        if (m_data) delete[] m_data;
    }

    void SpvArrayWidget::Data(unsigned char* dataPtr) const {
        memcpy(dataPtr, m_data, m_type->size);
    }

    void SpvArrayWidget::Fill(const unsigned char* data) {
        memcpy(m_data, data, m_type->size);
    }

    void SpvArrayWidget::InitData() {
        HandleArrayElementChange();
    }

    void SpvArrayWidget::OnRelease() {
        m_inputArea->Clear();
    }

    void SpvArrayWidget::HandleArrayElementChange() {
        int newIndex = 0;
        size_t lengthMult = 1;
        for (int i = m_dimensionIndices.size() - 1; i >= 0; --i) {
            newIndex += int(m_dimensionIndices[i] * lengthMult);
            lengthMult *= m_type->lengths[i];
        }
        newIndex *= int(m_stride);

        m_inputLeaf->Data(&m_data[m_activeWidgetIdx]);
        m_activeWidgetIdx = newIndex;
        m_inputLeaf->Fill(&m_data[m_activeWidgetIdx]);
    }
}
