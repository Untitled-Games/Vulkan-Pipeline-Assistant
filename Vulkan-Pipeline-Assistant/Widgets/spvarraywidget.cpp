#include "spvarraywidget.h"

#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCoreApplication>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvstructwidget.h"
#include "spvresourcewidget.h"
#include "../Vulkan/spirvresource.h"

namespace vpa {
    SpvArrayWidget::SpvArrayWidget(ContainerWidget* cont, SpvResourceWidget* resourceWidget, SpvArrayType* type, QWidget* parent)
        : SpvWidget(cont, resourceWidget, parent), m_type(type),  m_data(nullptr), m_totalNumElements(0), m_activeWidgetIdx(0) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);

        m_infoGroup = new QWidget(parent);
        m_infoGroup->setLayout(new QHBoxLayout(m_infoGroup));
        m_infoGroup->layout()->setAlignment(Qt::AlignTop);

        m_inputArea = new ContainerWidget(parent);

        QString infoStr;
        infoStr.append(QStringLiteral("Array(%1) %2D ").arg(SpvTypeNameStrings[size_t(m_type->subtype->Type())]).arg(m_type->lengths.size()));
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
            m_dimensionIndices[i] = 0;
            QSpinBox* indexEdit = new QSpinBox(parent);
            indexEdit->setRange(0, int(m_type->lengths[i]) - 1);
            indexEdit->setMaximumWidth(40);
            m_indicesGroup->layout()->addWidget(indexEdit);
            QObject::connect(indexEdit, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){
                m_dimensionIndices[i] = size_t(indexEdit->value());
                HandleArrayElementChange();
                m_resourceWidget->WriteDescriptorData();
            });
        }

        m_data = new unsigned char[m_type->size];
        memset(m_data, 0, m_type->size);
        m_stride = m_type->subtype->size;

        m_inputWidget = MakeSpvWidget(m_type->subtype, m_inputArea, m_resourceWidget);

        layout->addWidget(m_infoGroup);
        layout->addWidget(m_indicesGroup);
        layout->addWidget(m_inputArea);
        setLayout(layout);

        hide();
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

    void SpvArrayWidget::OnDrawerInit() {
        if (m_type->subtype->Type() == SpvTypeName::Struct) {
            m_inputWidget->SetDrawerItemWidget(GetDrawerItemWidget());
            m_inputWidget->OnDrawerInit();
        }
    }

    void SpvArrayWidget::Init() {
        m_inputWidget->Init();
        HandleArrayElementChange();
    }

    void SpvArrayWidget::OnClick(bool expanding) {
        if (expanding) {
            m_container->ShowWidget(this);
            if (m_type->subtype->Type() == SpvTypeName::Matrix || m_type->subtype->Type() == SpvTypeName::Vector) {
                m_inputArea->ShowWidget(m_inputWidget);
            }
            else if (m_type->subtype->Type() == SpvTypeName::Struct) {
                SpvWidget* defaultStructWidget = reinterpret_cast<SpvStructWidget*>(m_inputWidget)->GetTypeWidget(0);
                if (defaultStructWidget) m_inputArea->ShowWidget(defaultStructWidget);
            }
        }
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

        m_inputWidget->Data(&m_data[m_activeWidgetIdx]);
        m_activeWidgetIdx = newIndex;
        m_inputWidget->Fill(&m_data[m_activeWidgetIdx]);
    }
}
