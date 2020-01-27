#ifndef TABBEDCONTAINERWIDGET_H
#define TABBEDCONTAINERWIDGET_H

#include <QWidget>

#include "../common.h"

class QPushButton;
namespace vpa {
    class ContainerWidget;

    enum class TabLayoutDirection {
        Vertical, Horizontal
    };

    class TabbedContainerWidget : public QWidget {
        Q_OBJECT
    public:
        TabbedContainerWidget(TabLayoutDirection layoutDirection, int tabWidth, int tabHeight, QWidget* parent);
        ~TabbedContainerWidget() override;

        void AddTab(QString tabName, QWidget* containerItem);
        void ClearContainer();

        void InitSize();
        void resizeEvent(QResizeEvent* event) override;

    private:
        int VisibleTabCount();
        void ShiftTabsLeft();
        void ShiftTabsRight();

        QWidget* m_tabArea;
        ContainerWidget* m_container;
        QVector<QPushButton*> m_tabs;
        QVector<QWidget*> m_containerItems;

        QPushButton* m_tabLeftButton;
        QPushButton* m_tabRightButton;
        bool m_arrowsVisible;

        TabLayoutDirection m_layoutDirection;
        int m_visibleTabCount;
        int m_leftTabIdx;
        int m_tabWidth;
        int m_tabHeight;

        static const int ArrowDim = 15;
    };
}

#endif // TABBEDCONTAINERWIDGET_H
