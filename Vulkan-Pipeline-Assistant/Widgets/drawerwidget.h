#ifndef DRAWERWIDGET_H
#define DRAWERWIDGET_H

#include <QVector>
#include <QQueue>
#include <QWidget>

#include "common.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
namespace vpa {

    class DrawerItem {
        friend class DrawerItemWidget;
    public:
        DrawerItem() = default;
        virtual ~DrawerItem() = default;
        virtual void OnExpand() { }
        virtual void OnClose() { }
        virtual void OnClick(bool) { } // Param is true if expanding
        virtual void OnDrawerInit() { } // Called when widget associated with this item is added to a drawer.

        DrawerItemWidget* GetDrawerItemWidget() { return m_widget; }
        void SetDrawerItemWidget(DrawerItemWidget* widget) { m_widget = widget; }

    protected:
        void AddChildWidget(DrawerItem* item, QString name, QColor baseColour);

    private:
        DrawerItemWidget* m_widget;
    };

    class DrawerItemWidget : public QWidget {
        friend class DrawerWidget;
        Q_OBJECT
    public:
        DrawerItemWidget(DrawerWidget* drawer, DrawerItem* item, QString name, QColor pallete);
        void Expand();
        void Close();
        DrawerItemWidget* AddChild(DrawerItem* item, QString name, QColor baseColour);
        DrawerItemWidget* AddChild(DrawerItemWidget* child);

    protected:
        void mousePressEvent(QMouseEvent* event) override;

    private:
        void RotateIcon(int rot);
        void AddChildrenToDrawer();

        QPixmap m_iconPixmap;
        int m_rootIdx;
        int m_drawerLayoutIdx;
        bool m_expanded;
        QLabel* m_label;
        QLabel* m_shadowLabel;
        QLabel* m_icon;
        DrawerWidget* m_drawer;
        DrawerItem* m_item;
        QVector<DrawerItemWidget*> m_children;
    };

    class DrawerWidget : public QWidget {
        Q_OBJECT
    public:
        DrawerWidget(QWidget* parent);
        ~DrawerWidget() = default;

        void AddRootItem(DrawerItemWidget* item);
        void AddChildWidget(int rootIdx, int layoutIdx, DrawerItemWidget* child);

        void ShowItem(DrawerItemWidget* item);
        void HideItem(DrawerItemWidget* item);

        void Clear();

    private:
        QVector<DrawerItemWidget*> m_rootWidgets;
        QVector<QVBoxLayout*> m_rootLayouts;
    };
}

#endif // DRAWERWIDGET_H
