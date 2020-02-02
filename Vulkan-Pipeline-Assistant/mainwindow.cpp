#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_vulkandockwidget.h"

#include <QPushButton>
#include <QFileDialog>
#include <QKeyEvent>
#include <QDockWidget>
#include <qt_windows.h>

#include "./Vulkan/pipelineconfig.h"
#include "./Vulkan/descriptors.h"
#include "./Widgets/containerwidget.h"
#include "./Widgets/descriptortree.h"

namespace vpa {
    QString VPAError::lastMessage = "";

    bool DockWidget::nativeEvent(const QByteArray &eventType, void* message, long* result) {
        Q_UNUSED(result)
        Q_UNUSED(eventType)
        if (QGuiApplication::platformName() == "windows") return m_window->WindowsNativeEvent(static_cast<MSG*>(message));
        return false;
    }

    QLineEdit* MainWindow::s_console = nullptr;

    QLineEdit* MainWindow::Console() {
        return s_console;
    }

    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), m_ui(new Ui::MainWindow), m_vulkan(nullptr), m_descriptorTree(nullptr) {
        m_ui->setupUi(this);
        s_console = m_ui->gtxConsole;
        m_ui->gtxVertexFileName->setText(SHADERDIR"vs_test.spv");
        m_ui->gtxFragmentFileName->setText(SHADERDIR"fs_test.spv");
        this->setCentralWidget(m_ui->centralwidget);

        m_descriptorTypeWidget = new ContainerWidget(m_ui->gwDescriptorContainer);
        m_ui->gwDescriptorContainer->layout()->addWidget(m_descriptorTypeWidget);

        m_ui->ShaderTabs->setCurrentIndex(0);
        m_ui->ConfigTabs->setCurrentIndex(0);

        m_vkDockWidget = new DockWidget(this);
        m_vkDockUi = new Ui::DockWidget();
        m_vkDockUi->setupUi(m_vkDockWidget);
        m_vkDockUi->gwDisplayArea->setLayout(new QHBoxLayout());
        m_vkDockWidget->show();
        m_vulkan = new VulkanMain(m_vkDockUi->gwDisplayArea, std::bind(&MainWindow::VulkanCreationCallback, this));

        m_vulkan->GetConfig().vertShader = SHADERDIR"vs_test.spv";
        m_vulkan->GetConfig().fragShader = SHADERDIR"fs_test.spv";

        ConnectInterface();
        // QObject::connect(m_cacheBtn, &QPushButton::released, [this](){ this->m_vulkan->WritePipelineCache(); });
    }

    MainWindow::~MainWindow() {
        delete m_ui;
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        delete m_vulkan;
        event->accept();
    }

    bool MainWindow::nativeEvent(const QByteArray &eventType, void* message, long* result) {
        Q_UNUSED(result)
        Q_UNUSED(eventType)
        if (QGuiApplication::platformName() == "windows") return WindowsNativeEvent(static_cast<MSG*>(message));
        return false;
    }

    bool MainWindow::WindowsNativeEvent(MSG* msg) {
        if (msg->message == WM_ENTERSIZEMOVE) {
            if (m_vulkan) m_vulkan->Details().window->HandlingResize(true);
            return true;
        }
        else if(msg->message == WM_EXITSIZEMOVE) {
            if (m_vulkan) {
                m_vulkan->Details().window->HandlingResize(false);
                if (m_vulkan->State() == VulkanState::Ok) m_vulkan->RecreateSwapchain();
            }
            return true;
        }
        return false;
    }

    void MainWindow::ConnectInterface() {
        // ------ Shader connections ------
        QObject::connect(m_ui->gbVertexFile, &QPushButton::released, [this](){
            QString str = QFileDialog::getOpenFileName(this, tr("Open File"), SHADERDIR, tr("Shader Files (*.spv)"));
            m_ui->gtxVertexFileName->setText(str);
            Config().vertShader = str;
            this->WriteAndReload(ReloadFlags::Everything);
        });
        QObject::connect(m_ui->gbFragmentFile, &QPushButton::released, [this](){
            QString str = QFileDialog::getOpenFileName(this, tr("Open File"), SHADERDIR, tr("Shader Files (*.spv)"));
            m_ui->gtxFragmentFileName->setText(str);
            Config().fragShader = str;
            this->WriteAndReload(ReloadFlags::Everything);
        });
        QObject::connect(m_ui->gbGeometryFile, &QPushButton::released, [this](){
            QString str = QFileDialog::getOpenFileName(this, tr("Open File"), SHADERDIR, tr("Shader Files (*.spv)"));
            m_ui->gtxGeometryFileName->setText(str);
            Config().geomShader = str;
            this->WriteAndReload(ReloadFlags::Everything);
        });
        QObject::connect(m_ui->gbTessControlFile, &QPushButton::released, [this](){
            QString str = QFileDialog::getOpenFileName(this, tr("Open File"), SHADERDIR, tr("Shader Files (*.spv)"));
            m_ui->gtxTessControlFileName->setText(str);
            Config().tescShader = str;
            this->WriteAndReload(ReloadFlags::Everything);
        });
        QObject::connect(m_ui->gbTessEvalFile, &QPushButton::released, [this](){
            QString str = QFileDialog::getOpenFileName(this, tr("Open File"), SHADERDIR, tr("Shader Files (*.spv)"));
            m_ui->gtxTessEvalFileName->setText(str);
            Config().teseShader = str;
            this->WriteAndReload(ReloadFlags::Everything);
        });

        // ------ Vertex Input connections ------
        QObject::connect(m_ui->gcbTopology, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            HandleConfigValueChange<VkPrimitiveTopology>(Config().writables.topology, ReloadFlags::Pipeline, index);
        });
        QObject::connect(m_ui->gcPrimitiveRestart, QOverload<int>::of(&QCheckBox::stateChanged), [this](int state){
            HandleConfigValueChange<VkBool32>(Config().writables.primitiveRestartEnable, ReloadFlags::Pipeline, state == 0 ? VK_FALSE : VK_TRUE);
        });
        QObject::connect(m_ui->gsPatchPointCount, QOverload<int>::of(&QSlider::valueChanged), [this](int value) {
           HandleConfigValueChange<uint32_t>(Config().writables.patchControlPoints, ReloadFlags::Pipeline, value);
        });
        // ------ Viewport connections ------
        // ------ Rasterizer connections ------
        QObject::connect(m_ui->gcbPolygonMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkPolygonMode>(Config().writables.polygonMode, ReloadFlags::Pipeline, index);
        });
        QObject::connect(m_ui->gcDiscardEnable, QOverload<int>::of(&QCheckBox::stateChanged), [this](int state){
            HandleConfigValueChange<VkBool32>(Config().writables.rasterizerDiscardEnable, ReloadFlags::Pipeline, state == 0 ? VK_FALSE : VK_TRUE);
        });
        QObject::connect(m_ui->gsLineWidth, QOverload<int>::of(&QSlider::valueChanged), [this](int value) {
           HandleConfigValueChange<float>(Config().writables.lineWidth, ReloadFlags::Pipeline, value);
        });
        QObject::connect(m_ui->gcbCullMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkCullModeFlagBits>(Config().writables.cullMode, ReloadFlags::Pipeline, index);
        });
        QObject::connect(m_ui->gcbFrontFace, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkFrontFace>(Config().writables.frontFace, ReloadFlags::Pipeline, index);
        });
        QObject::connect(m_ui->gcDepthClamp, QOverload<int>::of(&QCheckBox::stateChanged), [this](int state){
            HandleConfigValueChange<VkBool32>(Config().writables.depthClampEnable, ReloadFlags::Pipeline, state == 0 ? VK_FALSE : VK_TRUE);
        });
        QObject::connect(m_ui->gcDepthBias, QOverload<int>::of(&QCheckBox::stateChanged), [this](int state){
            HandleConfigValueChange<VkBool32>(Config().writables.depthBiasEnable, ReloadFlags::Pipeline, state == 0 ? VK_FALSE : VK_TRUE);
        });
        QObject::connect(m_ui->gsDepthBiasConstant, QOverload<int>::of(&QSlider::valueChanged), [this](int value) {
           HandleConfigValueChange<float>(Config().writables.depthBiasConstantFactor, ReloadFlags::Pipeline, value);
        });
        QObject::connect(m_ui->gsDepthBiasClamp, QOverload<int>::of(&QSlider::valueChanged), [this](int value) {
           HandleConfigValueChange<float>(Config().writables.depthBiasClamp, ReloadFlags::Pipeline, value);
        });
        QObject::connect(m_ui->gsDepthBiasSlope, QOverload<int>::of(&QSlider::valueChanged), [this](int value) {
           HandleConfigValueChange<float>(Config().writables.depthBiasSlopeFactor, ReloadFlags::Pipeline, value);
        });
        // ------ Multisample connections ------
        QObject::connect(m_ui->gcbSampleCount, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkSampleCountFlagBits>(Config().writables.msaaSamples, ReloadFlags::RenderPass, index);
        });
        QObject::connect(m_ui->gsMinSampleShading, QOverload<int>::of(&QSlider::valueChanged), [this](int value) {
            HandleConfigValueChange<float>(Config().writables.minSampleShading, ReloadFlags::RenderPass, value);
        });
        // ------ Depth Stencil connections ------
        QObject::connect(m_ui->gcDepthTestEnable, QOverload<int>::of(&QCheckBox::stateChanged), [this](int state){
            HandleConfigValueChange<VkBool32>(Config().writables.depthTestEnable, ReloadFlags::Pipeline, state == 0 ? VK_FALSE : VK_TRUE);
        });
        QObject::connect(m_ui->gcDepthWriteEnable, QOverload<int>::of(&QCheckBox::stateChanged), [this](int state){
            HandleConfigValueChange<VkBool32>(Config().writables.depthWriteEnable, ReloadFlags::Pipeline, state == 0 ? VK_FALSE : VK_TRUE);
        });
        QObject::connect(m_ui->gcDepthTestEnable, QOverload<int>::of(&QCheckBox::stateChanged), [this](int state){
            HandleConfigValueChange<VkBool32>(Config().writables.depthBoundsTest, ReloadFlags::Pipeline, state == 0 ? VK_FALSE : VK_TRUE);
        });
        QObject::connect(m_ui->gcDepthStencilEnable, QOverload<int>::of(&QCheckBox::stateChanged), [this](int state){
            HandleConfigValueChange<VkBool32>(Config().writables.stencilTestEnable, ReloadFlags::Pipeline, state == 0 ? VK_FALSE : VK_TRUE);
        });
        QObject::connect(m_ui->gcbDepthCompareOp, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkCompareOp>(Config().writables.depthCompareOp, ReloadFlags::Pipeline, index);
        });
        // ------ Renderpass connections ------
    }

    void MainWindow::HandleViewChangeApply(QVector<QLineEdit*> v) {
        PipelineConfig& config = this->m_vulkan->GetConfig();
        VkViewport& viewport = config.viewports[0];
        viewport.x = v.at(0)->text().toFloat();
        viewport.y = v.at(1)->text().toFloat();
        viewport.width = v.at(2)->text().toFloat();
        viewport.height = v.at(3)->text().toFloat();
        viewport.minDepth = v.at(4)->text().toFloat();
        viewport.maxDepth = v.at(5)->text().toFloat();
    }

    void MainWindow::HandleViewChangeReset(QVector<QLineEdit*> v) {
        /*if (defaultShader != "") shaderConfig = defaultShader;

        QWidget* container = new QWidget(parent);

        QLabel* label = new QLabel(labelStr, container);

        QLineEdit* field = new QLineEdit(defaultShader, container);
        field->setReadOnly(true);

        QPushButton* dialogBtn = new QPushButton(container);
        dialogBtn->setIcon(QIcon(IMAGEDIR"file_dialog.svg"));

        QHBoxLayout* layout = new QHBoxLayout(container);
        layout->addWidget(label);
        layout->addWidget(field);
        layout->addWidget(dialogBtn);

        container->setLayout(layout);
        parent->layout()->addWidget(container);

        QObject::connect(dialogBtn, &QPushButton::released, [this, field, &shaderConfig](){
            QString str = QFileDialog::getOpenFileName(this, tr("Open File"), ".", tr("Shader Files (*.spv)"));
            field->setText(str);
            shaderConfig = field->text();
            this->WriteAndReload(ReloadFlags::Everything);
        });*/
    }

    QWidget* MainWindow::MakeViewportStateBlock() {
        // ----------- Viewport -------------
        QWidget* container = new QWidget();
        QLabel* xLabel = new QLabel("Viewport X", container);
        QLineEdit* xBox = new QLineEdit(container);
        xBox->setValidator(new QDoubleValidator(0.0, 10000.0, 2, container));
        QLabel* yLabel = new QLabel("Viewport Y", container);
        QLineEdit* yBox = new QLineEdit(container);
        yBox->setValidator(new QDoubleValidator(0.0, 10000.0, 2, container));
        QLabel* widthLabel = new QLabel("Viewport Width", container);
        QLineEdit* widthBox = new QLineEdit(container);
        widthBox->setValidator(new QDoubleValidator(0.0, 10000.0, 2, container));
        QLabel* heightLabel = new QLabel("Viewport Height", container);
        QLineEdit* heightBox = new QLineEdit(container);
        heightBox->setValidator(new QDoubleValidator(0.0, 10000.0, 2, container));
        QLabel* minDepthLabel = new QLabel("Min Depth", container);
        QLineEdit* minDepthBox = new QLineEdit(container);
        minDepthBox->setValidator(new QDoubleValidator(0.0, 10000.0, 2, container));
        QLabel* maxDepthLabel = new QLabel("Max Depth", container);
        QLineEdit* maxDepthBox = new QLineEdit(container);
        maxDepthBox->setValidator(new QDoubleValidator(0.0, 10000.0, 2, container));

        QVector<QLineEdit*> viewportBoxes;
        viewportBoxes.push_back(xBox);
        viewportBoxes.push_back(yBox);
        viewportBoxes.push_back(widthBox);
        viewportBoxes.push_back(heightBox);
        viewportBoxes.push_back(minDepthBox);
        viewportBoxes.push_back(maxDepthBox);

        QPushButton* resetViewChangeButton = new QPushButton("Reset Changes", container);
        QObject::connect(resetViewChangeButton, &QPushButton::released, [this, viewportBoxes] {
            this->HandleViewChangeReset(viewportBoxes);
        });
        QPushButton* applyViewChangeButton = new QPushButton("Apply Changes", container);
        QObject::connect(applyViewChangeButton, &QPushButton::released, [this, viewportBoxes] {
            this->HandleViewChangeApply(viewportBoxes);
        });

        QGridLayout* layout = new QGridLayout(container);
        container->setLayout(layout);

        layout->addWidget(xLabel, 0, 0);
        layout->addWidget(xBox, 1, 0);
        layout->addWidget(yLabel, 0, 1);
        layout->addWidget(yBox, 1, 1);
        layout->addWidget(widthLabel, 0, 2);
        layout->addWidget(widthBox, 1, 2);
        layout->addWidget(heightLabel, 2, 0);
        layout->addWidget(heightBox, 3, 0);
        layout->addWidget(minDepthLabel, 2, 1);
        layout->addWidget(minDepthBox, 3, 1);
        layout->addWidget(maxDepthLabel, 2, 2);
        layout->addWidget(maxDepthBox, 3, 2);
        layout->addWidget(resetViewChangeButton, 6, 2);
        layout->addWidget(applyViewChangeButton, 6, 3);

        // ----------- Scissor -------------
        QLabel* scissorXLabel = new QLabel("Scissor X", container);
        QLineEdit* scissorXBox = new QLineEdit(container);
        scissorXBox->setValidator(new QIntValidator(0, 10000, container));
        QLabel* scissorYLabel = new QLabel("Scissor Y", container);
        QLineEdit* scissorYBox = new QLineEdit(container);
        scissorYBox->setValidator(new QIntValidator(0, 10000, container));
        QLabel* scissorWidthLabel = new QLabel("Scissor Width", container);
        QLineEdit* scissorWidthBox = new QLineEdit(container);
        scissorWidthBox->setValidator(new QIntValidator(0, 10000, container));
        QLabel* scissorHeightLabel = new QLabel("Scissor Height", container);
        QLineEdit* scissorHeightBox = new QLineEdit(container);
        scissorHeightBox->setValidator(new QIntValidator(0, 10000, container));

        layout->addWidget(scissorXLabel, 4, 0);
        layout->addWidget(scissorXBox, 5, 0);
        layout->addWidget(scissorYLabel, 4, 1);
        layout->addWidget(scissorYBox, 5, 1);
        layout->addWidget(scissorWidthLabel, 4, 2);
        layout->addWidget(scissorWidthBox, 5, 2);
        layout->addWidget(scissorHeightLabel, 4, 3);
        layout->addWidget(scissorHeightBox, 5, 3);

        return container;
    }

    QWidget* MainWindow::MakeRenderPassBlock() {
        QWidget* container = new QWidget();
        QLabel* subpassLabel = new QLabel("Subpass Index", container);
        QLineEdit* subpassBox = new QLineEdit("0", container);
        subpassBox->setValidator(new QIntValidator(0, 9, subpassBox));

        QGridLayout* layout = new QGridLayout(container);
        container->setLayout(layout);

        layout->addWidget(subpassLabel, 0, 0);
        layout->addWidget(subpassBox, 1, 0);

        return container;
    }

    void MainWindow::MakeDescriptorBlock() {
        m_descriptorTypeWidget->Clear();
        delete m_descriptorTree;
        m_descriptorTree = nullptr;
        if (!m_vulkan) return;

        Descriptors* descriptors = m_vulkan->GetDescriptors();
        if (!descriptors) return;
        m_descriptorTree = new DescriptorTree(m_ui->gtDescriptors, m_ui->gtxDescriptorGroupInfo, m_ui->gtxDescriptorTypeInfo, m_descriptorTypeWidget, descriptors);

        for (auto& set : descriptors->Buffers().keys()) {
            for (int i = 0; i < descriptors->Buffers()[set].size(); ++i) {
                SpvResource* resource = descriptors->Buffers()[set][i].descriptor.resource;
                m_descriptorTree->CreateDescriptorWidgetTree(set, i, resource);
            }
        }
        for (auto& set : descriptors->Buffers().keys()) {
            for (int i = 0; i < descriptors->Images()[set].size(); ++i) {
                SpvResource* resource = descriptors->Images()[set][i].descriptor.resource;
                m_descriptorTree->CreateDescriptorWidgetTree(set, i, resource);
            }
        }
        for (auto& pushConstant : descriptors->PushConstants().values()) {
            SpvResource* resource = pushConstant.resource;
            m_descriptorTree->CreateDescriptorWidgetTree(0, 0, resource);
        }
    }

    void MainWindow::SetupDisplayAttachments() {
        m_vkDockUi->gcbAttachment->clear();
        if (!m_vulkan) return;

        m_vkDockUi->gcbAttachment->addItems(m_vulkan->AttachmentNames());

        // TODO connect to change attachment to display
    }

    QComboBox* MainWindow::MakeComboBox(QWidget* parent, QVector<QString> items) {
        QComboBox* box = new QComboBox(parent);
        for (QString& str : items) {
            box->addItem(str);
        }
        return box;
    }

    void MainWindow::VulkanCreationCallback() {
        MakeDescriptorBlock();
        SetupDisplayAttachments();
    }

    void MainWindow::WriteAndReload(ReloadFlags flag) const {
        this->m_vulkan->WritePipelineConfig();
        this->m_vulkan->Reload(flag);
    }

    PipelineConfig& MainWindow::Config() const {
        return this->m_vulkan->GetConfig();
    }
}
