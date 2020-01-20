#include "containerwidget.h"

#include <QLayout>

namespace vpa {
    ContainerWidget::ContainerWidget(QWidget* parent) : QWidget(parent), m_currentWidget(nullptr) {
        setLayout(new QVBoxLayout());
    }

    void ContainerWidget::ShowWidget(QWidget* widget) {
        if (widget == m_currentWidget) return;

        Clear();
        m_currentWidget = widget;
        layout()->addWidget(m_currentWidget);
        m_currentWidget->show();
    }

    void ContainerWidget::Clear() {
        if (m_currentWidget) {
            m_currentWidget->hide();
            layout()->removeWidget(m_currentWidget);
            ContainerItem* citem = dynamic_cast<ContainerItem*>(m_currentWidget);
            if (citem) {
                citem->OnRelease();
            }
            m_currentWidget = nullptr;
        }
    }
}
