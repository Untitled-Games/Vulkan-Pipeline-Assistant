#ifndef WIDGETARRAY_H
#define WIDGETARRAY_H

#include "spvwidget.h"

namespace vpa {
    struct SpvArrayType;
    class SpvArrayWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvArrayWidget(SpvArrayType* type, QWidget* parent);
        ~SpvArrayWidget() override;

        void Data(unsigned char* dataPtr) const override;
        void Fill(unsigned char* data) override;

    private slots:
        void HandleArrayElementChange();

    private:
        SpvArrayType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_indicesGroup;
        SpvWidget* m_inputWidget;
        QVector<size_t> m_dimensionIndices;
        unsigned char* m_data;

        size_t m_totalNumElements;
        size_t m_stride;
        int m_activeWidgetIdx;
    };
}

#endif // WIDGETARRAY_H
