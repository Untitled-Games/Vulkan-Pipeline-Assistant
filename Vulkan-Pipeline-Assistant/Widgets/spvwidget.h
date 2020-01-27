#ifndef SPVWIDGET_H
#define SPVWIDGET_H

#include <QWidget>
#include <QEvent>

#include "containerwidget.h"
#include "drawerwidget.h"

namespace vpa {
    struct SpvType;
    class Descriptors;
    class SpvResourceWidget;

    class SpvWidget : public QWidget, public DrawerItem, public ContainerItem {
        Q_OBJECT
    public:
        SpvWidget(ContainerWidget* cont, SpvResourceWidget* resourceWidget, QWidget* parent = nullptr) : QWidget(parent), m_container(cont), m_resourceWidget(resourceWidget) { }
        virtual ~SpvWidget() override = default;

        // Get data from the widget, passed in to dataPtr
        virtual void Data(unsigned char* dataPtr) const = 0;

        // Fill the widget with data
        virtual void Fill(const unsigned char* data) = 0;

        // Called after it becomes safe to write descriptor data
        virtual void Init() { }

        virtual void OnClick(bool expanding) override;
        virtual void OnRelease() override { }

        static SpvWidget* MakeSpvWidget(SpvType* type, ContainerWidget* container, SpvResourceWidget* dataCallback);

    protected:
        ContainerWidget* m_container;
        SpvResourceWidget* m_resourceWidget;
    };
}

#endif // SPVWIDGET_H
