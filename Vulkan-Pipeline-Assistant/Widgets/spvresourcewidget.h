#ifndef SPVRESOURCE_H
#define SPVRESOURCE_H

#include "spvwidget.h"

namespace vpa {
    struct SpvResource;
    struct DescriptorInfo;
    class Descriptors;

    class SpvResourceWidget : public SpvWidget {
    public:
        SpvResourceWidget(ContainerWidget* cont, Descriptors* descriptors, SpvResource* resource, uint32_t set, int index, QWidget* parent);

        QString Title() const;

        void Data(unsigned char* dataPtr) const override;
        void Fill(const unsigned char* data) override;
        void OnDrawerInit() override;
        void OnClick(bool expanding) override;

        void WriteDescriptorData();
        void WriteDescriptorData(QString fileName);

    private:
        SpvResource* m_resource;
        QWidget* m_groupWidget;
        SpvWidget* m_typeWidget;
        Descriptors* m_descriptors;
        uint32_t m_set;
        int m_index;
    };
}

#endif // SPVRESOURCE_H
