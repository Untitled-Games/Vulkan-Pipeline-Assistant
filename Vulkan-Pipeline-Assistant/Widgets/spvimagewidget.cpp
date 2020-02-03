#include "spvimagewidget.h"

#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QFileDialog>
#include <QCoreApplication>

#include "../Vulkan/spirvresource.h"
#include "../Vulkan/descriptors.h"
#include "descriptortree.h"

namespace vpa {
    SpvImageWidget::SpvImageWidget(SpvImageType* type, DescriptorNodeRoot* root)
        : SpvWidget(root), m_type(type) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);

        QPushButton* imgPreview = new QPushButton(this);
        imgPreview->setMinimumSize(200, 200);
        imgPreview->setIcon(QIcon(TEXDIR"default.png"));
        imgPreview->setIconSize(QSize(200, 200));
        layout->addWidget(imgPreview);
        QObject::connect(imgPreview, &QPushButton::pressed, [this, imgPreview]{
            QString imgFileName = QFileDialog::getOpenFileName(this, tr("Open File"), ".", tr("Image Files (*.png)"));
            if (imgFileName != "") {
                imgPreview->setIcon(QIcon(imgFileName));
                imgPreview->setIconSize(QSize(200, 200));
                m_root->WriteDescriptorData(imgFileName);
            }
        });

        setLayout(layout);
    }
}
