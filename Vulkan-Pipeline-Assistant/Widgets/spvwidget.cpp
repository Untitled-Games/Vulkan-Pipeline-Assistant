#include "spvwidget.h"

#include "../Vulkan/spirvresource.h"
#include "spvarraywidget.h"
#include "spvvectorwidget.h"
#include "spvimagewidget.h"
#include "spvmatrixwidget.h"
#include "spvstructwidget.h"

namespace vpa {
    bool SpvWidget::event(QEvent* event) {
        if (event->type() == SpvGuiDataChangeType || event->type() == SpvGuiImageChangeType) {
            event->ignore();
            return false;
        }
        else {
            return true;
        }
    }

    void SpvWidget::OnClick(bool expanding) {
        if (expanding) {
            m_container->ShowWidget(this);
        }
    }

    SpvWidget* SpvWidget::MakeSpvWidget(SpvType* type, ContainerWidget* container) {
        switch (type->Type()) {
        case SpvTypeName::Vector:
            return new SpvVectorWidget(container, reinterpret_cast<SpvVectorType*>(type), container);
        case SpvTypeName::Matrix:
            return new SpvMatrixWidget(container, reinterpret_cast<SpvMatrixType*>(type), container);
        case SpvTypeName::Struct:
            return new SpvStructWidget(container, reinterpret_cast<SpvStructType*>(type), container);
        case SpvTypeName::Array:
            return new SpvArrayWidget(container, reinterpret_cast<SpvArrayType*>(type), container);
        case SpvTypeName::Image:
            return new SpvImageWidget(container, reinterpret_cast<SpvImageType*>(type), container);
        case SpvTypeName::Count_:
            qWarning("Unknown spv widget type.");
            return nullptr;
        }
    }
}
