#ifndef WIDGETVECTOR_H
#define WIDGETVECTOR_H

#include "spvwidget.h"

class QLineEdit;
namespace vpa {
    struct SpvVectorType;
    class SpvVectorWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvVectorWidget(SpvVectorType* type, QWidget* parent);

        void Data(unsigned char* dataPtr) const override;
        void Fill(unsigned char* data) override;

    private slots:
        void HandleNormalize();

    private:
        SpvVectorType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_inputsGroup;
        QLineEdit* m_inputs[4];

        static const QString VecStrings[4];
        static const QString ComponentStrings[4];
    };
}

#endif // WIDGETVECTOR_H
