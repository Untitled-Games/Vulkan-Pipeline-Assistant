#ifndef SPVWIDGET_H
#define SPVWIDGET_H

#include <QWidget>
#include <QEvent>

#include "containerwidget.h"
#include "drawerwidget.h"

#define SPV_DATA_CHANGE_EVENT(parent) QEvent evnt = QEvent(SpvGuiDataChangeType); \
    QCoreApplication::sendEvent(parent, &evnt)

#define SPV_IMAGE_CHANGE_EVENT(parent, str) ImageChangeEvent evnt = ImageChangeEvent(str); \
    QCoreApplication::sendEvent(parent, &evnt)

namespace vpa {
    struct SpvType;
    constexpr QEvent::Type SpvGuiDataChangeType = QEvent::Type(int(QEvent::Type::User) + 1);
    constexpr QEvent::Type SpvGuiImageChangeType = QEvent::Type(int(QEvent::Type::User) + 2);

    class ImageChangeEvent : public QEvent {
    public:
        ImageChangeEvent(QString fileName) : QEvent(SpvGuiImageChangeType), m_fileName(fileName) {  }
        const QString& FileName() const { return m_fileName; }

    private:
        QString m_fileName;
    };

    class SpvWidget : public QWidget, public DrawerItem, public ContainerItem {
        Q_OBJECT
    public:
        SpvWidget(ContainerWidget* cont, QWidget* parent = nullptr) : QWidget(parent), m_container(cont) { }
        virtual ~SpvWidget() override = default;

        // Get data from the widget, passed in to dataPtr
        virtual void Data(unsigned char* dataPtr) const = 0;

        // Fill the widget with data
        virtual void Fill(const unsigned char* data) = 0;

        virtual bool event(QEvent* event) override;
        virtual void OnClick(bool expanding) override;
        virtual void OnRelease() override { }

        static SpvWidget* MakeSpvWidget(SpvType* type, ContainerWidget* container);

    protected:
        ContainerWidget* m_container;
    };
}

#endif // SPVWIDGET_H
