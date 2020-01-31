#ifndef SOLOCONTAINERWIDGET_H
#define SOLOCONTAINERWIDGET_H

#include <QWidget>

#include "../common.h"

namespace vpa {
    class ContainerItem : public QWidget {
    public:
        ContainerItem(QWidget* parent = nullptr) : QWidget(parent) { }
        virtual ~ContainerItem() = default;
        // Called When the container clears the widget from the container
        virtual void OnRelease() { }
    };

    // This class defines a very simple area container which controls hiding an existing widget to display a new one
    class ContainerWidget : public QWidget {
        Q_OBJECT
    public:
        ContainerWidget(QWidget* parent);
        virtual ~ContainerWidget() = default;

        void ShowWidget(QWidget* widget);
        void Clear();

    private:
        QWidget* m_currentWidget;
    };
}

#endif // SOLOCONTAINERWIDGET_H
