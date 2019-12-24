/*
 * Author: Ralph Ridley
 * Date: 19/12/19
 * Functionality for the main window and UI.
 */

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
    private slots:
        void HandleShaderFileDialog(QLineEdit* field);
        void HandleConfigAreaChange(int toIdx);
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

        QComboBox* MakeComboBox(QWidget* parent, QVector<QString> items);

        Ui::MainWindow* m_ui;
        VulkanMain* m_vulkan;
        QPushButton* m_createCacheBtn;
        QGridLayout* m_layout;

        QWidget* m_leftColumnContainer;
        QWidget* m_rightTopContainer;
        QWidget* m_rightBottomContainer;

        QPushButton* m_cacheBtn;

        QVector<QPushButton*> m_configButtons;
        QVector<QWidget*> m_configBlocks;

        int m_unhiddenIdx;
    };
}
#endif // MAINWINDOW_H
