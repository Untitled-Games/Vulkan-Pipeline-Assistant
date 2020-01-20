#ifndef WIDGETMATRIX_H
#define WIDGETMATRIX_H

#include "spvwidget.h"

class QLineEdit;
namespace vpa {
    struct SpvMatrixType;

    class SpvMatrixWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvMatrixWidget(ContainerWidget* cont, SpvMatrixType* type, QWidget* parent);

        void Data(unsigned char* dataPtr) const override;
        void Fill(const unsigned char* data) override;
        void OnClick(bool expanding) override;

    private slots:
        void HandleInverse();

    private:
        SpvMatrixType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_inputsGroup;
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
