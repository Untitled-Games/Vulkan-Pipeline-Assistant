#ifndef WIDGETMATRIX_H
#define WIDGETMATRIX_H

#include "spvwidget.h"

class QLineEdit;
namespace vpa {
    struct SpvMatrixType;
    class SpvMatrixWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvMatrixWidget(SpvMatrixType* type, QWidget* parent = nullptr);

        void Data(unsigned char* dataPtr) const override;
        void Fill(unsigned char* data) override;

    private:
        SpvMatrixType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_inputsGroup;
        QLineEdit* m_inputs[4][4];
    };
}

#endif // WIDGETMATRIX_H
