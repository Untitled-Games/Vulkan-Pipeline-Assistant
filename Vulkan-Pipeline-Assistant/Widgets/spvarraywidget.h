#ifndef WIDGETARRAY_H
#define WIDGETARRAY_H

#include "spvwidget.h"

namespace vpa {
    struct SpvArrayType;
    struct DescriptorNodeLeaf;
    class ContainerWidget;

    class SpvArrayWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvArrayWidget(SpvArrayType* type, DescriptorNodeRoot* root, ContainerWidget*& subContainer);
        ~SpvArrayWidget() override;
        void OnRelease() override;

        void SetChildLeaf(DescriptorNodeLeaf* leaf) { m_inputLeaf = leaf; }
    protected:
        void Data(unsigned char* dataPtr) const override;
        void Fill(const unsigned char* data) override;
        void InitData() override;

    private slots:
        void HandleArrayElementChange();

    private:
        SpvArrayType* m_type;
        QWidget* m_indicesGroup;
        DescriptorNodeLeaf* m_inputLeaf;
        ContainerWidget* m_inputArea;
        QVector<size_t> m_dimensionIndices;
        unsigned char* m_data;

        size_t m_totalNumElements;
        size_t m_stride;
        int m_activeWidgetIdx;
    };
}

#endif // WIDGETARRAY_H
