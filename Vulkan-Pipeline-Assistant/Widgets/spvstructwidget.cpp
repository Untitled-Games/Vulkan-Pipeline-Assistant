#include "spvstructwidget.h"

#include <QPushButton>
#include <QLabel>
#include <QLayout>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvarraywidget.h"
#include "spvstructwidget.h"
#include "../Vulkan/spirvresource.h"

namespace vpa {
    SpvStructWidget::SpvStructWidget(ContainerWidget* cont, SpvStructType* type, QWidget* parent)
        : SpvWidget(cont, parent), m_type(type), m_activeIndex(0) {
        QVBoxLayout* layout = new QVBoxLayout(this);

        m_infoGroup = new QWidget(parent);
        m_infoGroup->setLayout(new QHBoxLayout(m_infoGroup));
        m_infoGroup->layout()->addWidget(new QLabel(QStringLiteral("struct"), parent));
        m_infoGroup->layout()->setAlignment(Qt::AlignTop);

        m_typeWidgets.resize(m_type->members.size());
        for (int i = 0; i < m_typeWidgets.size(); ++i) {
            m_typeWidgets[i] = MakeSpvWidget(m_type->members[i], m_container);
            m_typeWidgets[i]->hide();
        }

        layout->addWidget(m_infoGroup);
        setLayout(layout);

        hide();
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

    void SpvStructWidget::OnDrawerInit() {
        for (int i = 0; i < m_typeWidgets.size(); ++i) {
            AddChildWidget(m_typeWidgets[i], m_type->members[i]->name, Qt::gray);
        }
    }

    void SpvStructWidget::HandleMemberButtonPress(int idx) {
        m_typeWidgets[m_activeIndex]->hide();
        m_activeIndex = idx;
        m_typeWidgets[m_activeIndex]->show();
    }
}
