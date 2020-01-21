#ifndef SPVSTRUCTWIDGET_H
#define SPVSTRUCTWIDGET_H

#include "spvwidget.h"

class QPushButton;
namespace vpa {
    struct SpvStructType;

    class SpvStructWidget : public SpvWidget {
    public:
        SpvStructWidget(ContainerWidget* cont, SpvStructType* type, QWidget* parent);

        void Data(unsigned char* dataPtr) const override;
        void Fill(const unsigned char* data) override;
        void OnDrawerInit() override;

        SpvWidget* GetTypeWidget(int i);

    private slots:
        void HandleMemberButtonPress(int idx);

    private:
        SpvStructType* m_type;
        QWidget* m_infoGroup;
        QWidget* m_typesGroup;
        QVector<QPushButton*> m_memberButtons;
        QVector<SpvWidget*> m_typeWidgets;
        int m_activeIndex;
    };
}

#endif // SPVSTRUCTWIDGET_H
