#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QLayout>
#include <QLineEdit>

#include "./Vulkan/vulkanmain.h"
#include "common.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPushButton;

namespace vpa {
    class DrawerWidget;
    class ContainerWidget;
    class TabbedContainerWidget;

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
        ~MainWindow() override;

        static QComboBox* MakeComboBox(QWidget* parent, QVector<QString> items);

        void closeEvent(QCloseEvent* event) override;
        bool nativeEvent(const QByteArray &eventType, void* message, long* result) override;

    public slots:
        void HandleViewChangeReset(QVector<QLineEdit*> v);
        void HandleViewChangeApply(QVector<QLineEdit*> v);
        void HandleConfigFloatTextChange(float& configVar, ReloadFlags reloadFlag, QLineEdit* editBox);

    private:
        bool WindowsNativeEvent(MSG* msg);
        void ConnectInterface();

        void MakeShaderBlock(QWidget* parent, QString labelStr, QString& shaderConfig, QString defaultShader = "");
        QWidget* MakeViewportStateBlock();
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
        QWidget* m_leftBottomArea;
        QWidget* m_rightBottomContainer;
        TabbedContainerWidget* m_configArea;
        QLineEdit* m_console;

        QWidget* m_descriptorArea;
        ContainerWidget* m_descriptorContainer;
        DrawerWidget* m_descriptorDrawer;

        QPushButton* m_cacheBtn;

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
