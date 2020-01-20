#ifndef WIDGETARRAY_H
#define WIDGETARRAY_H

#include "spvwidget.h"

namespace vpa {
    struct SpvArrayType;
    class ContainerWidget;

    class SpvArrayWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvArrayWidget(ContainerWidget* cont, SpvArrayType* type, QWidget* parent);
        ~SpvArrayWidget() override;

        void Data(unsigned char* dataPtr) const override;
        void Fill(const unsigned char* data) override;
        void OnDrawerInit() override;
        void OnClick(bool expanding) override;
        void OnRelease() override;

    private slots:
        void HandleArrayElementChange();

    private:
        SpvArrayType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_indicesGroup;
        SpvWidget* m_inputWidget;
        ContainerWidget* m_inputArea;
        QVector<size_t> m_dimensionIndices;
        unsigned char* m_data;

        size_t m_totalNumElements;
        size_t m_stride;
        int m_activeWidgetIdx;
    };
}

#endif // WIDGETARRAY_H
