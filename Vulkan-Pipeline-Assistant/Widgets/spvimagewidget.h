#ifndef SPVIMAGEWIDGET_H
#define SPVIMAGEWIDGET_H

#include "spvwidget.h"

namespace vpa {
    struct SpvImageType;

    class SpvImageWidget : public SpvWidget {
    public:
        SpvImageWidget(SpvImageType* type, DescriptorNodeRoot* root);

        void Data(unsigned char*) const override { }
        void Fill(const unsigned char*) override { }

    private:
        SpvImageType* m_type;
    };
}

#endif // SPVIMAGEWIDGET_H
