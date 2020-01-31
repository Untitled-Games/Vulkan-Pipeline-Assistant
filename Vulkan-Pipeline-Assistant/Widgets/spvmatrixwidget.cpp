#include "spvmatrixwidget.h"

#include <QLineEdit>
#include <QLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QComboBox>
#include <QMatrix4x4>

#include "../Vulkan/spirvresource.h"
#include "../Vulkan/descriptors.h"
#include "descriptortree.h"
#include "mainwindow.h"

namespace vpa {
    SpvMatrixWidget::SpvMatrixWidget(SpvMatrixType* type, DescriptorNodeRoot* root)
        : SpvWidget(root), m_type(type) {
        QGridLayout* layout = new QGridLayout(this);
        layout->setAlignment(Qt::AlignTop);

        QPushButton* invBtn = new QPushButton("Inverse", this);
        connect(invBtn, SIGNAL(pressed()), this, SLOT(HandleInverse()));
        layout->addWidget(invBtn, 0, 0);

        if (m_type->rows == 4 && m_type->columns == 4) {
            QComboBox* defaultMatricesBox = MainWindow::MakeComboBox(this, { "Model", "View", "Projection", "MVP" });
            layout->addWidget(defaultMatricesBox, 0, 1);
            QPushButton* defaultMatrixbutton = new QPushButton("Apply");
            layout->addWidget(defaultMatrixbutton, 0, 2);
            QObject::connect(defaultMatrixbutton, &QPushButton::pressed, [this, defaultMatricesBox] {
                if (defaultMatricesBox->currentIndex() == 0) this->Fill(BYTE_CPTR(Descriptors::DefaultModelMatrix().data()));
                if (defaultMatricesBox->currentIndex() == 1) this->Fill(BYTE_CPTR(Descriptors::DefaultViewMatrix().data()));
                if (defaultMatricesBox->currentIndex() == 2) this->Fill(BYTE_CPTR(Descriptors::DefaultProjectionMatrix().data()));
                if (defaultMatricesBox->currentIndex() == 3) this->Fill(BYTE_CPTR(Descriptors::DefaultMVPMatrix().data()));
            });
        }

        for (size_t row = 0; row < m_type->rows; ++row) {
            for (size_t col = 0; col < m_type->columns; ++col) {
                m_inputs[row][col] = new QLineEdit(this);
                layout->addWidget(m_inputs[row][col], int(row) + 1, int(col));
                QObject::connect(m_inputs[row][col], &QLineEdit::textChanged, [this] { m_root->WriteDescriptorData(); });
            }
        }

        setLayout(layout);
    }

    void SpvMatrixWidget::Data(unsigned char* dataPtr) const {
        float* floatPtr = reinterpret_cast<float*>(dataPtr);
        for (size_t row = 0; row < m_type->rows; ++row) {
            for (size_t col = 0; col < m_type->columns; ++col) {
                floatPtr[row * m_type->columns + col] = m_inputs[row][col]->text().toFloat();
            }
        }
    }

    void SpvMatrixWidget::Fill(const unsigned char* data) {
        const float* floatPtr = reinterpret_cast<const float*>(data);
        for (size_t row = 0; row < m_type->rows; ++row) {
            for (size_t col = 0; col < m_type->columns; ++col) {
                m_inputs[row][col]->setText(QString::number(double(floatPtr[row * m_type->columns + col])));
            }
        }
    }

    void SpvMatrixWidget::InitData() {
        Fill(BYTE_CPTR(DefaultData));
        if (m_type->name == "mvp") {
            Fill(BYTE_CPTR(Descriptors::DefaultMVPMatrix().data()));
        }
    }

    void SpvMatrixWidget::HandleInverse() {
        float* data = new float[m_type->rows * m_type->columns];
        for (size_t row = 0; row < m_type->rows; ++row) {
            for (size_t col = 0; col < m_type->columns; ++col) {
                data[row * m_type->columns + col] = m_inputs[row][col]->text().toFloat();
            }
        }
        QMatrix4x4 mat(data, int(m_type->columns), int(m_type->rows));
        mat = mat.inverted();
        Fill(BYTE_CPTR(mat.data()));
    }
}
