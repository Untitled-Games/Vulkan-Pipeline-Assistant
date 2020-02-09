#ifndef INFORMATIVESLIDER_H
#define INFORMATIVESLIDER_H

#include <QSlider>

#include "common.h"

namespace vpa {
    class InformativeSlider : public QSlider {
        Q_OBJECT
    public:
        InformativeSlider(QWidget* parent = nullptr);
        InformativeSlider(Qt::Orientation orientation, QWidget* parent = nullptr);

   protected:
       void sliderChange(SliderChange change) override;
    };
}

#endif // INFORMATIVESLIDER_H
