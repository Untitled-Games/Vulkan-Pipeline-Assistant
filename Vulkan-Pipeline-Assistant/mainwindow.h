#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>

#include "./Vulkan/vulkanmain.h"
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

    public slots:
        void HandleViewChangeReset(QVector<QLineEdit*> v);
        void HandleViewChangeApply(QVector<QLineEdit*> v);

    private:
        bool WindowsNativeEvent(MSG* msg);
        void ConnectInterface();

        QWidget* MakeViewportStateBlock();
        QWidget* MakeRenderPassBlock();
        void MakeDescriptorBlock();
        void SetupDisplayAttachments();

        void LoadShaderText(QPlainTextEdit* textEdit, QString name);
        void WriteShaderText(QPlainTextEdit* textEdit, QString name);

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
