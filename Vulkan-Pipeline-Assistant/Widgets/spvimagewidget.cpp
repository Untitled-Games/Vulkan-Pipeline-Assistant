#include "spvimagewidget.h"

#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QFileDialog>
#include <QCoreApplication>

#include "../Vulkan/spirvresource.h"
#include "../Vulkan/descriptors.h"

namespace vpa {
    SpvImageWidget::SpvImageWidget(SpvImageType* type, QWidget* parent)
        : SpvWidget(parent), m_type(type) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);

        m_infoGroup = new QWidget(parent);
        m_infoGroup->setLayout(new QHBoxLayout(m_infoGroup));
        m_infoGroup->layout()->setAlignment(Qt::AlignTop);
        m_infoGroup->layout()->addWidget(new QLabel("image " + SpvImageTypeNameStrings[size_t(m_type->imageTypename)], parent)); // TODO more info

        m_inputsGroup = new QWidget(parent);
        QGridLayout* inputsGroupLayout = new QGridLayout(m_inputsGroup);
        inputsGroupLayout->setAlignment(Qt::AlignTop);
        QPushButton* imgPreview = new QPushButton(parent);
        imgPreview->setMinimumSize(200, 200);
        imgPreview->setIcon(QIcon(TEXDIR"default.png"));
        imgPreview->setIconSize(QSize(200, 200));
        inputsGroupLayout->addWidget(imgPreview);
        QObject::connect(imgPreview, &QPushButton::pressed, [this, parent, imgPreview]{
            QString imgFileName = QFileDialog::getOpenFileName(this, tr("Open File"), ".", tr("Image Files (*.png)"));
            if (imgFileName != "") {
                imgPreview->setIcon(QIcon(imgFileName));
                imgPreview->setIconSize(QSize(200, 200));
                SPV_IMAGE_CHANGE_EVENT(parent, imgFileName);
            }
        });

        m_inputsGroup->setLayout(inputsGroupLayout);
        layout->addWidget(m_infoGroup);
        layout->addWidget(m_inputsGroup);
        setLayout(layout);
    }
}
