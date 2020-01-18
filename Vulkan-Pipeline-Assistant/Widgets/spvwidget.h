#ifndef SPVWIDGET_H
#define SPVWIDGET_H

#include <QWidget>
#include <QEvent>

#include "../common.h"
#include "drawerwidget.h"

#define SPV_DATA_CHANGE_EVENT(parent) QEvent evnt = QEvent(SpvGuiDataChangeType); \
    QCoreApplication::sendEvent(parent, &evnt)

#define SPV_IMAGE_CHANGE_EVENT(parent, str) ImageChangeEvent evnt = ImageChangeEvent(str); \
    QCoreApplication::sendEvent(parent, &evnt)

namespace vpa {
    constexpr QEvent::Type SpvGuiDataChangeType = QEvent::Type(int(QEvent::Type::User) + 1);
    constexpr QEvent::Type SpvGuiImageChangeType = QEvent::Type(int(QEvent::Type::User) + 2);

    class ImageChangeEvent : public QEvent {
    public:
        ImageChangeEvent(QString fileName) : QEvent(SpvGuiImageChangeType), m_fileName(fileName) {}
        const QString& FileName() const { return m_fileName; }
    private:
        QString m_fileName;
    };

    class SpvWidget : public QWidget, public DrawerItem {
        Q_OBJECT
    public:
        SpvWidget(QWidget* parent = nullptr) : QWidget(parent) { }
        virtual ~SpvWidget() override = default;

        // Get data from the widget, passed in to dataPtr
        virtual void Data(unsigned char* dataPtr) const = 0;

        // Fill the widget with data
        virtual void Fill(const unsigned char* data) = 0;

        virtual bool event(QEvent* event) override {
            if (event->type() == SpvGuiDataChangeType || event->type() == SpvGuiImageChangeType) {
                event->ignore();
                return false;
            }
            else {
                return true;
            }
        }
    };
}

#endif // SPVWIDGET_H
