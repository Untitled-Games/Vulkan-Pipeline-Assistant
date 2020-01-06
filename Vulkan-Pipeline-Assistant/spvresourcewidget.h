#ifndef SPVRESOURCE_H
#define SPVRESOURCE_H

#include "spvwidget.h"

namespace vpa {
    struct SpvResource;
    struct DescriptorInfo;
    class Descriptors;
    class SpvResourceWidget : public SpvWidget {
    public:
        SpvResourceWidget(Descriptors* descriptors, SpvResource* resource, uint32_t set, int index, QWidget* parent = nullptr);

        QString Title() const;

        void Data(unsigned char* dataPtr) const override;
        void Fill(unsigned char* data) override;

        bool event(QEvent* event) override;

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
