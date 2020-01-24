#include "spvwidget.h"

#include "../Vulkan/spirvresource.h"
#include "spvarraywidget.h"
#include "spvvectorwidget.h"
#include "spvimagewidget.h"
#include "spvmatrixwidget.h"
#include "spvstructwidget.h"

namespace vpa {
    void SpvWidget::OnClick(bool expanding) {
        if (expanding) {
            m_container->ShowWidget(this);
        }
    }

    SpvWidget* SpvWidget::MakeSpvWidget(SpvType* type, ContainerWidget* container, SpvResourceWidget* resourceWidget) {
        switch (type->Type()) {
        case SpvTypeName::Vector:
            return new SpvVectorWidget(container, resourceWidget, reinterpret_cast<SpvVectorType*>(type), container);
        case SpvTypeName::Matrix:
            return new SpvMatrixWidget(container, resourceWidget, reinterpret_cast<SpvMatrixType*>(type), container);
        case SpvTypeName::Struct:
            return new SpvStructWidget(container, resourceWidget, reinterpret_cast<SpvStructType*>(type), container);
        case SpvTypeName::Array:
            return new SpvArrayWidget(container, resourceWidget, reinterpret_cast<SpvArrayType*>(type), container);
        case SpvTypeName::Image:
            return new SpvImageWidget(container, resourceWidget, reinterpret_cast<SpvImageType*>(type), container);
        case SpvTypeName::Count_:
        default:
            qWarning("Unknown spv widget type.");
        }
        return nullptr;
    }
}
