#ifndef SPVSTRUCTWIDGET_H
#define SPVSTRUCTWIDGET_H

#include "spvwidget.h"

class QPushButton;
namespace vpa {
    struct SpvStructType;

    class SpvStructWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvStructWidget(SpvStructType* type);
    protected:
        void Data(unsigned char* dataPtr) const override { Q_UNUSED(dataPtr) }
        void Fill(const unsigned char* data) override { Q_UNUSED(data) }
        void InitData() override { }

    private:
        SpvStructType* m_type;
    };
}

#endif // SPVSTRUCTWIDGET_H
