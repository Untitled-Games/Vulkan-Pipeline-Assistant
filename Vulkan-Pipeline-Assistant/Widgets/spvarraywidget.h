#ifndef WIDGETARRAY_H
#define WIDGETARRAY_H

#include "spvwidget.h"

namespace vpa {
    struct SpvArrayType;
    class ContainerWidget;

    class SpvArrayWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvArrayWidget(SpvArrayType* type, DescriptorNodeRoot* root, ContainerWidget*& subContainer) : SpvWidget(root) {}
        ~SpvArrayWidget() override {}
        void OnRelease() override {}
    protected:
        void Data(unsigned char* dataPtr) const override {}

        // Fill the widget with data
        void Fill(const unsigned char* data) override {}

        // Called after it becomes safe to write descriptor data
        void InitData() override {}

    private slots:
        void HandleArrayElementChange() {}

    private:
        SpvArrayType* m_type;
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
