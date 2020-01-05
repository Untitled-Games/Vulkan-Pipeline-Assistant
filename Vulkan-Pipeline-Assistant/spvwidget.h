#ifndef SPVWIDGET_H
#define SPVWIDGET_H

#include <QWidget>

namespace vpa {
    class SpvWidget : public QWidget {
        Q_OBJECT
    public:
        SpvWidget(QWidget* parent = nullptr) : QWidget(parent) { }
        virtual ~SpvWidget() = default;

        // Get data from the widget, passed in to dataPtr
        virtual void Data(unsigned char* dataPtr) const = 0;

        // Fill the widget with data
        virtual void Fill(unsigned char* data) = 0;
    };
}

#endif // SPVWIDGET_H
