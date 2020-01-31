#ifndef WIDGETVECTOR_H
#define WIDGETVECTOR_H

#include "spvwidget.h"

class QLineEdit;
namespace vpa {
    struct SpvVectorType;

    class SpvVectorWidget : public SpvWidget {
        Q_OBJECT
    public:
        SpvVectorWidget(SpvVectorType* type, DescriptorNodeRoot* root);
    protected:
        void Data(unsigned char* dataPtr) const override;

        // Fill the widget with data
        void Fill(const unsigned char* data) override;

        // Called after it becomes safe to write descriptor data
        void InitData() override;

    private slots:
        void HandleNormalize();

    private:
        SpvVectorType* m_type;
        QLineEdit* m_inputs[4];

        static const QString ComponentStrings[4];
        static constexpr float DefaultData[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    };
}

#endif // WIDGETVECTOR_H
