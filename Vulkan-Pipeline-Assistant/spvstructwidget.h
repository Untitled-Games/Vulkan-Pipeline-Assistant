#ifndef SPVSTRUCTWIDGET_H
#define SPVSTRUCTWIDGET_H

#include "spvwidget.h"

class QPushButton;
namespace vpa {
    struct SpvStructType;
    class SpvStructWidget : public SpvWidget {
    public:
        SpvStructWidget(SpvStructType* type, QWidget* parent = nullptr);

        void Data(unsigned char* dataPtr) const override;
        void Fill(unsigned char* data) override;

    private slots:
        void HandleMemberButtonPress(int idx);

    private:
        SpvStructType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_inputsGroup;
        QWidget* m_buttonsGroup;
        QWidget* m_typesGroup;
        QVector<QPushButton*> m_memberButtons;
        QVector<SpvWidget*> m_typeWidgets;
        int m_activeIndex;
    };
}

#endif // SPVSTRUCTWIDGET_H
