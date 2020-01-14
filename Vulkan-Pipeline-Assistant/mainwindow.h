#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QLayout>
#include <QLineEdit>

#include "../Vulkan/vulkanmain.h"
#include "common.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPushButton;

namespace vpa {
    class MainWindow : public QMainWindow {
        Q_OBJECT

        struct ShaderBlock {
            QString text;
            QWidget* container;
            QLayout* layout;
            QLabel* label;
            QLineEdit* field;
            QPushButton* dialogBtn;
        };

        struct ConfigBlock {
            QLabel* title;
            QWidget* container;
            QVector<QPair<QLabel*, QWidget*>> options;
        };
    public:
        MainWindow(QWidget* parent = nullptr);
        ~MainWindow();

        static QComboBox* MakeComboBox(QWidget* parent, QVector<QString> items);
    private slots:
        void HandleConfigAreaChange(int toIdx);
        void HandleViewChangeReset(QVector<QLineEdit*> v);
        void HandleViewChangeApply(QVector<QLineEdit*> v);
        void HandleConfigFloatTextChange(float& configVar, ReloadFlags reloadFlag, QLineEdit* editBox);

    private:
        void CreateInterface();
        void AddConfigButtons();
        void AddConfigBlocks();

        void MakeShaderBlock(QWidget* parent, QString labelStr, QString& shaderConfig, QString defaultShader = "");
        QWidget* MakeVertexInputBlock();
        QWidget* MakeViewportStateBlock();
        QWidget* MakeRasterizerBlock();
        QWidget* MakeMultisampleBlock();
        QWidget* MakeDepthStencilBlock();
        QWidget* MakeRenderPassBlock();
        void MakeDescriptorBlock();
        template<typename T>
        T* MakeConfigWidget(QWidget* parent, QString name, int labRow, int labCol, int boxRow, int boxCol, T* inputBox);

        void VulkanCreationCallback();
        void WriteAndReload(ReloadFlags flag) const;
        PipelineConfig& Config() const;

        template<typename T>
        void HandleConfigValueChange(T& configVar, ReloadFlags reloadFlag, int index);

        Ui::MainWindow* m_ui;
        VulkanMain* m_vulkan;
        QPushButton* m_createCacheBtn;
        QGridLayout* m_layout;

        QWidget* m_masterContainer;
        QWidget* m_leftColumnContainer;
        QWidget* m_rightTopContainer;
        QWidget* m_rightBottomContainer;

        QPushButton* m_cacheBtn;

        QVector<QPushButton*> m_configButtons;
        QVector<QWidget*> m_configBlocks;

        int m_descriptorBlockIdx;
        int m_unhiddenIdx;
        int m_activeDescriptorIdx;

        static const QVector<QString> BoolComboOptions;
    };

    template<typename T>
    inline void MainWindow::HandleConfigValueChange(T& configVar, ReloadFlags reloadFlag, int index) {
        configVar = T(index);
        WriteAndReload(reloadFlag);
    }

    template<typename T>
    inline T* MainWindow::MakeConfigWidget(QWidget* parent, QString name, int labRow, int labCol, int boxRow, int boxCol, T* inputBox) {
        QLabel* label = new QLabel(name, parent);
        reinterpret_cast<QGridLayout*>(parent->layout())->addWidget(label, labRow, labCol);
        reinterpret_cast<QGridLayout*>(parent->layout())->addWidget(inputBox, boxRow, boxCol);
        return inputBox;
    }
}
#endif // MAINWINDOW_H
