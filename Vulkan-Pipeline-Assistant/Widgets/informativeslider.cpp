#include "informativeslider.h"

#include <QStyleOptionSlider>
#include <QToolTip>

namespace vpa {
    InformativeSlider::InformativeSlider(QWidget* parent) : QSlider(parent)  { }

    InformativeSlider::InformativeSlider(Qt::Orientation orientation, QWidget * parent) : QSlider(orientation, parent) { }

    void InformativeSlider::sliderChange(QAbstractSlider::SliderChange change) {
        QSlider::sliderChange(change);
        if (change == QAbstractSlider::SliderValueChange) {
            QStyleOptionSlider opt;
            initStyleOption(&opt);

            QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
            QPoint bottomRightCorner = sr.bottomLeft();

            QToolTip::showText(mapToGlobal(QPoint(bottomRightCorner.x(), bottomRightCorner.y())), QString::number(value()), this);
        }
    }
}
