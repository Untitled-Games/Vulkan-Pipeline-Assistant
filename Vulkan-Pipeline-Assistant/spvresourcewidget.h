#ifndef SPVRESOURCE_H
#define SPVRESOURCE_H

#include "spvwidget.h"

namespace vpa {
    struct SpvResource;
    class SpvResourceWidget : public SpvWidget {
    public:
        SpvResourceWidget(SpvResource* resource, QWidget* parent = nullptr);

        QString Title() const;

        void Data(unsigned char* dataPtr) const override;
        void Fill(unsigned char* data) override;

    private:
        SpvResource* m_resource;
        QWidget* m_groupWidget;
        SpvWidget* m_typeWidget;
    };
}

#endif // SPVRESOURCE_H
