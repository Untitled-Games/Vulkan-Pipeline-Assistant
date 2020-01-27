#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>
#include <QFileDialog>
#include <qt_windows.h>

#include "Vulkan/pipelineconfig.h"
#include "Vulkan/descriptors.h"
#include "Widgets/spvresourcewidget.h"
#include "Widgets/drawerwidget.h"
#include "Widgets/containerwidget.h"
#include "Widgets/tabbedcontainerwidget.h"

namespace vpa {
    QString VPAError::lastMessage = "";
    const QVector<QString> MainWindow::BoolComboOptions = { "False", "True" };

    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), m_ui(new Ui::MainWindow), m_vulkan(nullptr) {
        m_ui->setupUi(this);

        m_masterContainer = new QWidget(this);
        m_masterContainer->setGeometry(10, 20, this->width() - 10, this->height() - 20);
        m_layout = new QGridLayout(m_masterContainer);

        m_leftBottomArea = new QWidget(m_masterContainer);

        m_console = new QLineEdit("Console", m_masterContainer);
        m_console->setReadOnly(true);
        m_console->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        m_console->setStyleSheet("background-color:#aa1111;");

        m_rightBottomContainer = new QWidget(m_masterContainer);
        m_rightBottomContainer->setLayout(new QVBoxLayout(m_rightBottomContainer));
        m_vulkan = new VulkanMain(m_rightBottomContainer, std::bind(&MainWindow::VulkanCreationCallback, this));
        CreateInterface();

        m_layout->addWidget(m_configArea, 0, 0);
        m_layout->addWidget(m_descriptorArea, 0, 1);
        m_layout->addWidget(m_leftBottomArea, 1, 0);
        m_layout->addWidget(m_rightBottomContainer, 1, 1);
        m_layout->addWidget(m_console, 2, 0, 1, 2);

        m_layout->setColumnStretch(0, 1);
        m_layout->setColumnStretch(1, 1);
        m_layout->setContentsMargins(0, 5, 0, 0);
        m_layout->setSpacing(0);
        m_masterContainer->setLayout(m_layout);
        this->setCentralWidget(m_masterContainer);
    }

    MainWindow::~MainWindow() {
        delete m_ui;
    }

    void MainWindow::resizeEvent(QResizeEvent* event) {
        Q_UNUSED(event)
        m_console->setMaximumWidth(width());
    }

    bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long* result) {
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

    void MainWindow::CreateInterface() {
        QWidget* shaderWidget = new QWidget();
        shaderWidget->setLayout(new QVBoxLayout(shaderWidget));
        MakeShaderBlock(shaderWidget, "Vertex", Config().vertShader, SHADERDIR"vs_test.spv");
        MakeShaderBlock(shaderWidget, "Fragment", Config().fragShader, SHADERDIR"fs_test.spv");
        MakeShaderBlock(shaderWidget, "Tess Control", Config().tescShader);
        MakeShaderBlock(shaderWidget, "Tess Eval", Config().teseShader);
        MakeShaderBlock(shaderWidget, "Geometry", Config().geomShader);

        m_configArea = new TabbedContainerWidget(TabLayoutDirection::Vertical, 60, 15, m_masterContainer);
        m_configArea->AddTab("Shaders", shaderWidget);
        m_configArea->AddTab("Vertex Input", MakeVertexInputBlock());
        m_configArea->AddTab("Viewport", MakeViewportStateBlock());
        m_configArea->AddTab("Rasterizer", MakeRasterizerBlock());
        m_configArea->AddTab("Multisample", MakeMultisampleBlock());
        m_configArea->AddTab("Depth Stencil", MakeDepthStencilBlock());
        m_configArea->AddTab("Render pass", MakeRenderPassBlock());
        m_configArea->layout()->setAlignment(Qt::AlignmentFlag::AlignTop);
        m_configArea->setMaximumSize(650, height() / 2);
        m_configArea->setMinimumSize(250, height() / 4);
        m_configArea->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        m_configArea->InitSize();

        m_descriptorArea = new QWidget(m_masterContainer);
        m_descriptorArea->setLayout(new QHBoxLayout(m_descriptorArea));
        m_descriptorArea->setStyleSheet("background-color:#ff0000");
        m_descriptorDrawer = new DrawerWidget(m_descriptorArea);
        m_descriptorDrawer->setStyleSheet("background-color:#00ff00");
        m_descriptorDrawer->setMinimumWidth(100);
        m_descriptorArea->layout()->setAlignment(Qt::AlignmentFlag::AlignTop);
        m_descriptorContainer = new ContainerWidget(m_descriptorArea);
        m_descriptorContainer->setStyleSheet("background-color:#0000ff");
        m_descriptorArea->layout()->addWidget(m_descriptorDrawer);
        m_descriptorArea->layout()->addWidget(m_descriptorContainer);
        m_descriptorArea->hide();
        //m_descriptorDrawer->show();
        //m_descriptorContainer->show();

        MakeDescriptorBlock();

        /*m_cacheBtn = new QPushButton("Create cache", m_leftColumnContainer); TODO add to file toolbar menu
        m_leftColumnContainer->layout()->addWidget(m_cacheBtn);
        QObject::connect(m_cacheBtn, &QPushButton::released, [this](){ this->m_vulkan->WritePipelineCache(); });*/
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
        PipelineConfig& config = this->m_vulkan->GetConfig();
        VkViewport viewport = config.viewports[0];
        v.at(0)->setText(QString::number(double(viewport.x)));
        v.at(1)->setText(QString::number(double(viewport.y)));
        v.at(2)->setText(QString::number(double(viewport.width)));
        v.at(3)->setText(QString::number(double(viewport.height)));
        v.at(4)->setText(QString::number(double(viewport.minDepth)));
        v.at(5)->setText(QString::number(double(viewport.maxDepth)));
    }

    void MainWindow::HandleConfigFloatTextChange(float& configVar, ReloadFlags reloadFlag, QLineEdit* editBox) {
        configVar = editBox->text().toFloat();
        WriteAndReload(reloadFlag);
    }

    void MainWindow::MakeShaderBlock(QWidget* parent, QString labelStr, QString& shaderConfig, QString defaultShader) {
        if (defaultShader != "") shaderConfig = defaultShader;

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
        });
    }

    QWidget* MainWindow::MakeVertexInputBlock() {
        QWidget* container = new QWidget();
        container->setLayout(new QGridLayout(container));

        // ----------- Topology -------------
        QComboBox* comboBox = MakeConfigWidget(container, "Topology", 0, 0, 1, 0, MakeComboBox(container, {
                "Point List", "Line List", "Line Strip", "Triangle List",
                "Triangle Strip", "Triangle Fan", "Line List With Adjacency", "Line Strip With Adjacency",
                "Triangle List With Adjacency", "Triangle Strip With Adjacency", "Patch List"
        }));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            HandleConfigValueChange<VkPrimitiveTopology>(Config().writables.topology, ReloadFlags::Pipeline, index);
        });

        // ----------- Primitive Restart -------------
        comboBox = MakeConfigWidget(container, "Primitive Restart", 0, 1, 1, 1, MakeComboBox(container, BoolComboOptions));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.primitiveRestartEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Patch Points -------------
        QSpinBox* spinBox = MakeConfigWidget(container, "Patch Points", 0, 2, 1, 2, new QSpinBox(container));
        spinBox->setRange(0, int(m_vulkan->Limits().maxTessellationPatchSize));
        QObject::connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
           HandleConfigValueChange<uint32_t>(Config().writables.patchControlPoints, ReloadFlags::Pipeline, value);
        });

        return container;
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

    QWidget* MainWindow::MakeRasterizerBlock() {
        QWidget* container = new QWidget();
        container->setLayout(new QGridLayout(container));

        // ----------- Polygon Mode -------------
        QComboBox* comboBox = MakeConfigWidget(container, "Polygon Mode", 0, 0, 1, 0, MakeComboBox(container, {"Fill", "Line", "Point"}));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkPolygonMode>(Config().writables.polygonMode, ReloadFlags::Pipeline, index);
        });

        // ----------- Rasterizer Discard -------------
        comboBox = MakeConfigWidget(container, "Discard Enable", 0, 1, 1, 1, MakeComboBox(container, BoolComboOptions));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.rasterizerDiscardEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Line Width -------------
        QLineEdit* lineEdit = MakeConfigWidget(container, "Line Width", 0, 2, 1, 2, new QLineEdit("1.0", container));
        QObject::connect(lineEdit, &QLineEdit::textChanged, [this, lineEdit]() {
           HandleConfigFloatTextChange(Config().writables.lineWidth, ReloadFlags::Pipeline, lineEdit);
        });

        // ----------- Cull Mode -------------
        comboBox = MakeConfigWidget(container, "Cull Mode", 0, 3, 1, 3, MakeComboBox(container, {"None", "Front", "Back", "Front and Back"}));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkCullModeFlagBits>(Config().writables.cullMode, ReloadFlags::Pipeline, index);
        });

        // ----------- Front Face -------------
        comboBox = MakeConfigWidget(container, "Front Face", 0, 4, 1, 4, MakeComboBox(container, {"Counter Clockwise", "Clockwise"}));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkFrontFace>(Config().writables.frontFace, ReloadFlags::Pipeline, index);
        });

        // ----------- Depth Clamp -------------
        comboBox = MakeConfigWidget(container, "Depth Clamp Enable", 2, 0, 3, 0, MakeComboBox(container, BoolComboOptions));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.depthClampEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Depth Bias -------------
        comboBox = MakeConfigWidget(container, "Depth Bias Enable", 2, 1, 3, 1, MakeComboBox(container, BoolComboOptions));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.depthBiasEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Depth Bias Constant -------------
        lineEdit = MakeConfigWidget(container, "Depth Bias Constant", 2, 2, 3, 2, new QLineEdit("1.0", container));
        lineEdit->setValidator(new QDoubleValidator(0.0, 9.0, 3, container));
        QObject::connect(lineEdit, &QLineEdit::textChanged, [this, lineEdit]() {
           HandleConfigFloatTextChange(Config().writables.depthBiasConstantFactor, ReloadFlags::Pipeline, lineEdit);
        });

        // ----------- Depth Bias Clamp -------------
        lineEdit = MakeConfigWidget(container, "Depth Bias Clamp", 2, 3, 3, 3, new QLineEdit("1.0", container));
        lineEdit->setValidator(new QDoubleValidator(0.0, 9.0, 3, container));
        QObject::connect(lineEdit, &QLineEdit::textChanged, [this, lineEdit]() {
           HandleConfigFloatTextChange(Config().writables.depthBiasClamp, ReloadFlags::Pipeline, lineEdit);
        });

        // ----------- Depth Bias Slope -------------
        lineEdit = MakeConfigWidget(container, "Depth Bias Slope", 2, 4, 3, 4, new QLineEdit("1.0", container));
        lineEdit->setValidator(new QDoubleValidator(0.0, 9.0, 3, container));
        QObject::connect(lineEdit, &QLineEdit::textChanged, [this, lineEdit]() {
           HandleConfigFloatTextChange(Config().writables.depthBiasSlopeFactor, ReloadFlags::Pipeline, lineEdit);
        });

        return container;
    }

    QWidget* MainWindow::MakeMultisampleBlock() {
        QWidget* container = new QWidget();
        container->setLayout(new QGridLayout(container));

        // ----------- Sample Count -------------
        QComboBox* comboBox = MakeConfigWidget(container, "Sample Count", 0, 0, 1, 0, MakeComboBox(container, {"1", "2", "4", "8", "16", "32", "64"}));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkSampleCountFlagBits>(Config().writables.msaaSamples, ReloadFlags::RenderPass, index);
        });

        // ----------- Min Sample Shading -------------
        QLineEdit* lineEdit = MakeConfigWidget(container, "Min Sample Shading", 0, 1, 1, 1, new QLineEdit("0.2", container));
        lineEdit->setValidator(new QDoubleValidator(0.0, 1.0, 3, container));
        QObject::connect(lineEdit, &QLineEdit::textChanged, [this, lineEdit]() {
            HandleConfigFloatTextChange(Config().writables.minSampleShading, ReloadFlags::RenderPass, lineEdit);
        });

        return container;
    }

    QWidget* MainWindow::MakeDepthStencilBlock() {
        QWidget* container = new QWidget();
        container->setLayout(new QGridLayout(container));

        // ----------- Depth Test -------------
        QComboBox* comboBox = MakeConfigWidget(container, "Test Enable", 0, 0, 1, 0, MakeComboBox(container, BoolComboOptions));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.depthTestEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Depth Write -------------
        comboBox = MakeConfigWidget(container, "Write Enable", 0, 1, 1, 1, MakeComboBox(container, BoolComboOptions));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.depthWriteEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Bounds Enable -------------
        comboBox = MakeConfigWidget(container, "Bounds Enable", 2, 0, 3, 0, MakeComboBox(container, BoolComboOptions));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.depthBoundsTest, ReloadFlags::Pipeline, index);
        });

        // ----------- Stencil Test -------------
        comboBox = MakeConfigWidget(container, "Stencil Test Enable", 2, 1, 3, 1, MakeComboBox(container, BoolComboOptions));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.stencilTestEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Compare Op -------------
        comboBox = MakeConfigWidget(container, "Compare Op", 4, 0, 5, 0, MakeComboBox(container, {
                "Never", "Less", "Equal", "Less or Equal", "Greater", "Not Equal", "Greater or Equal", "Always" }));
        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkCompareOp>(Config().writables.depthCompareOp, ReloadFlags::Pipeline, index);
        });

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
        m_descriptorContainer->Clear();
        m_descriptorDrawer->Clear();
        m_descriptorContainer->show();
        m_descriptorDrawer->show();

        if (!m_vulkan) return;

        Descriptors* descriptors = m_vulkan->GetDescriptors();
        if (!descriptors) return;

        for (auto& set : descriptors->Buffers().keys()) {
            for (int i = 0; i < descriptors->Buffers()[set].size(); ++i) {
                SpvResource* resource = descriptors->Buffers()[set][i].descriptor.resource;
                m_descriptorDrawer->AddRootItem(new DrawerItemWidget(m_descriptorDrawer,
                    new SpvResourceWidget(m_descriptorContainer, descriptors, resource, set, i, m_descriptorContainer), resource->name, Qt::gray));
            }
        }
        for (auto& set : descriptors->Buffers().keys()) {
            for (int i = 0; i < descriptors->Images()[set].size(); ++i) {
                SpvResource* resource = descriptors->Images()[set][i].descriptor.resource;
                m_descriptorDrawer->AddRootItem(new DrawerItemWidget(m_descriptorDrawer,
                    new SpvResourceWidget(m_descriptorContainer, descriptors, resource, set, i, m_descriptorContainer), resource->name, Qt::gray));
            }
        }
        for (auto& pushConstant : descriptors->PushConstants().values()) {
            SpvResource* resource = pushConstant.resource;
            m_descriptorDrawer->AddRootItem(new DrawerItemWidget(m_descriptorDrawer,
                new SpvResourceWidget(m_descriptorContainer, descriptors, resource, 0, 0, m_descriptorContainer), resource->name, Qt::gray));
        }
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
    }

    void MainWindow::WriteAndReload(ReloadFlags flag) const {
        this->m_vulkan->WritePipelineConfig();
        this->m_vulkan->Reload(flag);
    }

    PipelineConfig& MainWindow::Config() const {
        return this->m_vulkan->GetConfig();
    }
}
