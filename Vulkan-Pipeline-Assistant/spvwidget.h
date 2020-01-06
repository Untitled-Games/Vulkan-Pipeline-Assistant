#ifndef SPVWIDGET_H
#define SPVWIDGET_H

#include <QWidget>
#include <QEvent>

#define SPV_DATA_CHANGE_EVENT(parent) QEvent evnt = QEvent(SpvGuiDataChangeType); \
    QCoreApplication::sendEvent(parent, &evnt)

namespace vpa {
    constexpr QEvent::Type SpvGuiDataChangeType = QEvent::Type(int(QEvent::Type::User) + 1);

    class SpvWidget : public QWidget {
        Q_OBJECT
    public:
        SpvWidget(QWidget* parent = nullptr) : QWidget(parent) { }
        virtual ~SpvWidget() = default;

        // Get data from the widget, passed in to dataPtr
        virtual void Data(unsigned char* dataPtr) const = 0;

        // Fill the widget with data
        virtual void Fill(unsigned char* data) = 0;

        virtual bool event(QEvent* event) override {
            if (event->type() == SpvGuiDataChangeType) {
                event->ignore();
                qDebug("Received event %i", event->type());
                return false;
            }
            else {
                return true;
            }
        }
    };
}

#endif // SPVWIDGET_H
