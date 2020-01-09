#include "spvmatrixwidget.h"

#include <QLineEdit>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QComboBox>
#include <QMatrix4x4>
#include <QValidator>

#include "spirvresource.h"
#include "mainwindow.h"
#include "descriptors.h"

namespace vpa {
    SpvMatrixWidget::SpvMatrixWidget(SpvMatrixType* type, QWidget* parent)
        : SpvWidget(parent), m_type(type) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);

        m_infoGroup = new QWidget(parent);
        m_infoGroup->setLayout(new QHBoxLayout(m_infoGroup));
        m_infoGroup->layout()->setAlignment(Qt::AlignTop);
        m_infoGroup->layout()->addWidget(new QLabel(QStringLiteral("mat %1x%2").arg(m_type->rows).arg(m_type->columns), parent));
        QPushButton* invBtn = new QPushButton("Inverse", m_infoGroup);
        m_infoGroup->layout()->addWidget(invBtn);
        connect(invBtn, SIGNAL(pressed()), this, SLOT(HandleInverse()));

        if (m_type->rows == 4 && m_type->columns == 4) {
            QComboBox* defaultMatricesBox = MainWindow::MakeComboBox(m_infoGroup, { "Model", "View", "Projection", "MVP" });
            m_infoGroup->layout()->addWidget(defaultMatricesBox);
            QPushButton* defaultMatrixbutton = new QPushButton("Apply");
            m_infoGroup->layout()->addWidget(defaultMatrixbutton);
            QObject::connect(defaultMatrixbutton, &QPushButton::pressed, [this, defaultMatricesBox] {
                if (defaultMatricesBox->currentIndex() == 0) this->Fill(BYTE_CPTR(Descriptors::DefaultModelMatrix().data()));
                if (defaultMatricesBox->currentIndex() == 1) this->Fill(BYTE_CPTR(Descriptors::DefaultViewMatrix().data()));
                if (defaultMatricesBox->currentIndex() == 2) this->Fill(BYTE_CPTR(Descriptors::DefaultProjectionMatrix().data()));
                if (defaultMatricesBox->currentIndex() == 3) this->Fill(BYTE_CPTR(Descriptors::DefaultMVPMatrix().data()));
            });
        }

        m_inputsGroup = new QWidget(parent);
        QGridLayout* inputsGroupLayout = new QGridLayout(m_inputsGroup);
        inputsGroupLayout->setAlignment(Qt::AlignTop);
        for (size_t row = 0; row < m_type->rows; ++row) {
            for (size_t col = 0; col < m_type->columns; ++col) {
                m_inputs[row][col] = new QLineEdit(m_inputsGroup);
                m_inputs[row][col]->setValidator(new QDoubleValidator(double(FLT_MIN + FLT_EPSILON), (double(FLT_MAX - FLT_EPSILON)), 4, m_inputs[row][col]));
                inputsGroupLayout->addWidget(m_inputs[row][col], int(row), int(col));
                QObject::connect(m_inputs[row][col], &QLineEdit::textChanged, [parent] { SPV_DATA_CHANGE_EVENT(parent); });
            }
        }

        m_inputsGroup->setLayout(inputsGroupLayout);
        layout->addWidget(m_infoGroup);
        layout->addWidget(m_inputsGroup);
        setLayout(layout);

        Fill(BYTE_CPTR(DefaultData));
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
