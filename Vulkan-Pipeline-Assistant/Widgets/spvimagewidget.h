#ifndef SPVIMAGEWIDGET_H
#define SPVIMAGEWIDGET_H

#include "spvwidget.h"

namespace vpa {
    struct SpvImageType;

    class SpvImageWidget : public SpvWidget {
    public:
        SpvImageWidget(ContainerWidget* cont, SpvImageType* type, QWidget* parent);

        void Data(unsigned char*) const override { }
        void Fill(const unsigned char*) override { }

    private:
        SpvImageType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_inputsGroup;
    };
}

#endif // SPVIMAGEWIDGET_H
