#include "drawerwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>

namespace vpa {

    void DrawerItem::AddChildWidget(DrawerItem* item, QString name, QColor baseColour) {
        m_widget->AddChild(item, name, baseColour);
    }

    DrawerItemWidget::DrawerItemWidget(DrawerWidget* drawer, DrawerItem* item, QString name, QColor baseColour)
        : QWidget(drawer), m_rootIdx(-1), m_drawerLayoutIdx(-1), m_expanded(false), m_icon(nullptr), m_drawer(drawer), m_item(item) {
        setLayout(new QHBoxLayout(this));
        setMaximumHeight(40);

        QPalette pallete = palette();
        pallete.setColor(QPalette::Background, baseColour);
        setPalette(pallete);
        setAutoFillBackground(true);
        setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);

        m_label = new QLabel(name, this);
        layout()->addWidget(m_label);

        item->m_widget = this;
        item->OnDrawerInit();
    }

    void DrawerItemWidget::Expand() {
        m_expanded = true;
        RotateIcon(90);
        m_item->OnExpand();
        m_drawer->ShowItem(this);
    }

    void DrawerItemWidget::Close() {
        if (!m_expanded) return;
        m_expanded = false;
        RotateIcon(0);
        m_item->OnClose();
        m_drawer->HideItem(this);
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
        if (m_rootIdx >= 0) {
            m_drawer->AddChildWidget(m_rootIdx, m_drawerLayoutIdx + m_children.size() - 1, child);
            child->AddChildrenToDrawer();
        }
        child->hide();
        return child;
    }

    void DrawerItemWidget::mousePressEvent(QMouseEvent*) {
        m_item->OnClick(!m_expanded);
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

    void DrawerItemWidget::AddChildrenToDrawer() {
        for (int i = 0; i < m_children.size(); ++i) {
            m_drawer->AddChildWidget(m_rootIdx, m_drawerLayoutIdx + i, m_children[i]);
            m_children[i]->AddChildrenToDrawer();
        }
    }

    DrawerWidget::DrawerWidget(QWidget* parent) : QWidget(parent), m_rootWidgets(0), m_rootLayouts(0) {
        setLayout(new QVBoxLayout(this));
        layout()->setAlignment(Qt::AlignmentFlag::AlignTop);
    }

    void DrawerWidget::AddRootItem(DrawerItemWidget* item) {
        item->m_rootIdx = m_rootWidgets.size();
        item->m_drawerLayoutIdx = 0;
        m_rootWidgets.push_back(item);
        m_rootLayouts.push_back(new QVBoxLayout());
        m_rootLayouts[item->m_rootIdx]->addWidget(item);
        layout()->addItem(m_rootLayouts[item->m_rootIdx]);
        item->AddChildrenToDrawer();
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

    void DrawerWidget::Clear() {
        for (int i = 0; i < m_rootWidgets.size(); ++i) {
            m_rootLayouts[i]->removeWidget(m_rootWidgets[i]);
            delete m_rootWidgets[i];
            layout()->removeItem(m_rootLayouts[i]);
            delete m_rootLayouts[i];
        }
        m_rootWidgets.clear();
        m_rootLayouts.clear();
    }
}
