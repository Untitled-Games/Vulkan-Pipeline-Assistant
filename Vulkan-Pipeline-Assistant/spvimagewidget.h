#ifndef SPVIMAGEWIDGET_H
#define SPVIMAGEWIDGET_H

#include "spvwidget.h"

namespace vpa {
    struct SpvImageType;
    class SpvImageWidget : public SpvWidget {
    public:
        SpvImageWidget(SpvImageType* type, QWidget* parent);

        void Data(unsigned char* dataPtr) const override { };
        void Fill(unsigned char* data) override { };

    private:
        SpvImageType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_inputsGroup;
    };
}

#endif // SPVIMAGEWIDGET_H
