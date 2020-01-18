#include "drawerwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>

namespace vpa {
    DrawerItemWidget::DrawerItemWidget(DrawerWidget* drawer, DrawerItem* item, QString name, QColor baseColour)
        : QWidget(drawer), m_rootIdx(-1), m_drawerLayoutIdx(-1), m_expanded(false), m_icon(nullptr), m_drawer(drawer), m_item(item) {
        setLayout(new QHBoxLayout(this));

        QPalette pallete = palette();
        pallete.setColor(QPalette::Background, baseColour);
        setPalette(pallete);
        setAutoFillBackground(true);
        setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);

        m_label = new QLabel(name, this);
        layout()->addWidget(m_label);
    }

    void DrawerItemWidget::Expand() {
        m_expanded = true;
        RotateIcon(90);
        if (!m_item->Expand() && !m_children.empty()) {
            m_drawer->ShowItem(this);
        }
    }

    void DrawerItemWidget::Close() {
        if (!m_expanded) return;
        m_expanded = false;
        RotateIcon(0);
        if (!m_item->Close() && !m_children.empty()) {
            m_drawer->HideItem(this);
        }
    }

    DrawerItemWidget* DrawerItemWidget::AddChild(DrawerItem* item, QString name, QColor baseColour) {
        return AddChild(new DrawerItemWidget(m_drawer, item, name, baseColour));
    }

    DrawerItemWidget* DrawerItemWidget::AddChild(DrawerItemWidget* child) {
        if (m_children.empty()) {
            m_icon = new QLabel(this);
            m_icon->setAlignment(Qt::AlignRight);
            m_iconPixmap = QPixmap(IMAGEDIR"right_chevron.svg");
            RotateIcon(0);
            layout()->addWidget(m_icon);
        }

        m_children.push_back(child);
        if (m_rootIdx >= 0) m_drawer->AddChildWidget(m_rootIdx, m_drawerLayoutIdx + m_children.size() - 1, child);
        child->hide();
        return child;
    }

    void DrawerItemWidget::mousePressEvent(QMouseEvent*) {
        if (m_expanded) Close();
        else Expand();
    }

    void DrawerItemWidget::RotateIcon(int rot) {
        if (m_icon) {
            QTransform trans;
            trans.rotate(rot);
            m_icon->setPixmap(m_iconPixmap.scaled(10, 10, Qt::KeepAspectRatio).transformed(trans));
        }
    }

    DrawerWidget::DrawerWidget(QWidget* parent) : QWidget(parent) {
        setLayout(new QVBoxLayout(this));
    }

    void DrawerWidget::AddRootItem(DrawerItemWidget* item) {
        item->m_rootIdx = m_rootWidgets.size();
        item->m_drawerLayoutIdx = 0;
        m_rootWidgets.push_back(item);
        m_rootLayouts.push_back(new QVBoxLayout());
        m_rootLayouts[item->m_rootIdx]->addWidget(item);
        layout()->addItem(m_rootLayouts[item->m_rootIdx]);
        for (int i = 0; i < item->m_children.size(); ++i) {
            AddChildWidget(item->m_rootIdx, item->m_drawerLayoutIdx + i, item->m_children[i]);
        }
    }

    void DrawerWidget::AddChildWidget(int rootIdx, int layoutIdx, DrawerItemWidget* child) {
        child->m_rootIdx = rootIdx;
        child->m_drawerLayoutIdx = layoutIdx + 1;
        m_rootLayouts[rootIdx]->insertWidget(child->m_drawerLayoutIdx, child);
    }

    void DrawerWidget::ShowItem(DrawerItemWidget* item) {
        for (DrawerItemWidget* child : item->m_children) {
            child->show();
            if (child->m_expanded) ShowItem(child);
        }
    }

    void DrawerWidget::HideItem(DrawerItemWidget* item) {
        for (DrawerItemWidget* child : item->m_children) {
            child->hide();
            HideItem(child);
        }
    }
}
