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
    class DescriptorTree;

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

    private:
        bool WindowsNativeEvent(MSG* msg);
        void ConnectInterface();

        QWidget* MakeViewportStateBlock();
        QWidget* MakeRenderPassBlock();
        void MakeDescriptorBlock();

        void VulkanCreationCallback();
        void WriteAndReload(ReloadFlags flag) const;
        PipelineConfig& Config() const;

        template<typename T>
        void HandleConfigValueChange(T& configVar, ReloadFlags reloadFlag, int index);

        Ui::MainWindow* m_ui;
        VulkanMain* m_vulkan;
        ContainerWidget* m_descriptorTypeWidget;
        DescriptorTree* m_descriptorTree;
    };

    template<typename T>
    inline void MainWindow::HandleConfigValueChange(T& configVar, ReloadFlags reloadFlag, int index) {
        configVar = T(index);
        WriteAndReload(reloadFlag);
    }
}
#endif // MAINWINDOW_H
