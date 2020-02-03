#ifndef WIDGETMATRIX_H
#define WIDGETMATRIX_H

#include "spvwidget.h"

class QLineEdit;
namespace vpa {
    struct SpvMatrixType;

    class SpvMatrixWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvMatrixWidget(SpvMatrixType* type, DescriptorNodeRoot* root);
    protected:
        void Data(unsigned char* dataPtr) const override;

        // Fill the widget with data
        void Fill(const unsigned char* data) override;

        // Called after it becomes safe to write descriptor data
        void InitData() override;

    private slots:
        void HandleInverse();

    private:
        SpvMatrixType* m_type;
        QLineEdit* m_inputs[4][4];

        static constexpr float DefaultData[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
    };
}

#endif // WIDGETMATRIX_H
