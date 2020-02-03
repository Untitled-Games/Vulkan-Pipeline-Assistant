#include "spvstructwidget.h"

#include <QPushButton>
#include <QLabel>
#include <QLayout>

#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvarraywidget.h"
#include "spvstructwidget.h"
#include "../Vulkan/spirvresource.h"

namespace vpa {
    SpvStructWidget::SpvStructWidget(SpvStructType* type)
        : SpvWidget(nullptr), m_type(type) { }
}
