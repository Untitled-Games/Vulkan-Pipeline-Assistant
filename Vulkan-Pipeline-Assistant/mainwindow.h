#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>

#include "Vulkan/vulkanmain.h"
#include "Vulkan/spirvresource.h"
#include "common.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
    class DockWidget;
}
QT_END_NAMESPACE

class QPushButton;
class QLineEdit;
class QComboBox;
class QPlainTextEdit;

namespace vpa {
    class DrawerWidget;
    class ContainerWidget;
    class TabbedContainerWidget;
    class DescriptorTree;
    class GLSLHighlighter;
    class CodeEditor;

    class MainWindow : public QMainWindow {
        Q_OBJECT
        friend class DockWidget;
    public:
        static QLineEdit* Console();

        MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

        static QComboBox* MakeComboBox(QWidget* parent, QVector<QString> items);

        void closeEvent(QCloseEvent* event) override;
        bool nativeEvent(const QByteArray &eventType, void* message, long* result) override;

        void PostVulkanSetup();

    public slots:
        void HandleViewChangeReset(QVector<QLineEdit*> v);
        void HandleViewChangeApply(QVector<QLineEdit*> v);

    protected:
        void keyPressEvent(QKeyEvent* event) override;

    private:
        bool WindowsNativeEvent(MSG* msg);
        void ConnectInterface();
        void ApplyLimits();

        QWidget* MakeViewportStateBlock();
        QWidget* MakeRenderPassBlock();
        void MakeDescriptorBlock();
        void SetupDisplayAttachments();

        void VulkanCreationCallback();
        void WriteAndReload(ReloadFlags flag) const;
        PipelineConfig& Config() const;

        template<typename T>
        void HandleConfigValueChange(T& configVar, ReloadFlags reloadFlag, int index);

        Ui::MainWindow* m_ui;
        VulkanMain* m_vulkan;
        ContainerWidget* m_descriptorTypeWidget;
        DescriptorTree* m_descriptorTree;

        QDockWidget* m_vkDockWidget;
        Ui::DockWidget* m_vkDockUi;

        GLSLHighlighter* m_glslHighlighters[5];
        CodeEditor* m_codeEditors[size_t(ShaderStage::Count_)];

        static QLineEdit* s_console;
    };

    class DockWidget : public QDockWidget {
        Q_OBJECT
    public:
        DockWidget(MainWindow* window) : QDockWidget(window), m_window(window) { }
        bool nativeEvent(const QByteArray &eventType, void* message, long* result) override;
    private:
        MainWindow* m_window;
    };

    template<typename T>
    inline void MainWindow::HandleConfigValueChange(T& configVar, ReloadFlags reloadFlag, int index) {
        configVar = T(index);
        WriteAndReload(reloadFlag);
    }
}
#endif // MAINWINDOW_H
