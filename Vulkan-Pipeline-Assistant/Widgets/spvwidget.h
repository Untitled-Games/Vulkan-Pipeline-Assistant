#ifndef SPVWIDGET_H
#define SPVWIDGET_H

#include <QTreeWidgetItem>
#include <QEvent>
#include <QTextEdit>

#include "containerwidget.h"
#include "../Vulkan/spirvresource.h"

namespace vpa {
    class SpvWidget;
    struct SpvResource;
    struct DescriptorNodeRoot;

    class SpvWidget : public ContainerItem {
        Q_OBJECT
    public:
        SpvWidget(DescriptorNodeRoot* root) : m_root(root) { }
        virtual ~SpvWidget() override = default;

        virtual void OnRelease() override { }

        virtual void Data(unsigned char* dataPtr) const = 0;

        // Fill the widget with data
        virtual void Fill(const unsigned char* data) = 0;

        // Called after it becomes safe to write descriptor data
        virtual void InitData() { }

        DescriptorNodeRoot* m_root;
    };
}
#endif // SPVWIDGET_H
