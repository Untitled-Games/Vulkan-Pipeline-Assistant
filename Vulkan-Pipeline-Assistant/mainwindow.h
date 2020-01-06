#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QFileDialog>
#include <QTextEdit>
#include <QLabel>
#include <QLayout>
#include <QComboBox>
#include <QPair>
#include "vulkanmain.h"

#define IMAGEDIR "../../Resources/Images/"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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
        void HandleShaderFileDialog(QLineEdit* field);
        void HandleConfigAreaChange(int toIdx);
        void HandleViewChangeReset(QVector<QLineEdit*> v);
        void HandleViewChangeApply(QVector<QLineEdit*> v);

    private:
        void AddConfigButtons();
        void AddConfigBlocks();

        void MakeShaderBlock(QWidget* parent, QString labelStr);
        QWidget* MakeVertexInputBlock();
        QWidget* MakeViewportStateBlock();
        QWidget* MakeRasterizerBlock();
        QWidget* MakeMultisampleBlock();
        QWidget* MakeDepthStencilBlock();
        QWidget* MakeRenderPassBlock();
        void MakeDescriptorBlock();

        void VulkanCreationCallback();

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
    };
}
#endif // MAINWINDOW_H
