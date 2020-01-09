#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>
#include <QFileDialog>

#include "pipelineconfig.h"
#include "descriptors.h"
#include "spvresourcewidget.h"

namespace vpa {
    const QVector<QString> MainWindow::BoolComboOptions = { "False", "True" };

    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), m_ui(new Ui::MainWindow), m_vulkan(nullptr), m_unhiddenIdx(0) {
        m_ui->setupUi(this);

        m_masterContainer = new QWidget(this);
        m_masterContainer->setGeometry(10, 20, this->width() - 10, this->height() - 20);
        m_layout = new QGridLayout(m_masterContainer);

        m_leftColumnContainer = new QWidget(m_masterContainer);
        m_leftColumnContainer->setLayout(new QVBoxLayout(m_leftColumnContainer));
        m_leftColumnContainer->layout()->setAlignment(Qt::AlignTop);

        m_rightTopContainer = new QWidget(m_masterContainer);

        m_rightBottomContainer = new QWidget(m_masterContainer);
        m_rightBottomContainer->setLayout(new QVBoxLayout(m_rightBottomContainer));
        m_vulkan = new VulkanMain(m_rightBottomContainer, std::bind(&MainWindow::VulkanCreationCallback, this), std::bind(&MainWindow::CreateInterface, this));

        m_layout->addWidget(m_leftColumnContainer, 0, 0);
        m_layout->addWidget(m_rightTopContainer, 0, 1);
        m_layout->addWidget(m_rightBottomContainer, 1, 1);

        m_layout->setColumnStretch(0, 1);
        m_layout->setColumnStretch(1, 4);
        m_masterContainer->setLayout(m_layout);
        this->setCentralWidget(m_masterContainer);
    }

    MainWindow::~MainWindow() {
        delete m_ui;
    }

    void MainWindow::CreateInterface() {
        AddConfigButtons();
        AddConfigBlocks();

        m_cacheBtn = new QPushButton("Create cache", m_leftColumnContainer);
        m_leftColumnContainer->layout()->addWidget(m_cacheBtn);
        QObject::connect(m_cacheBtn, &QPushButton::released, [this]{
            this->m_vulkan->WritePipelineCache();
        });
    }

    void MainWindow::AddConfigButtons() {
        m_configButtons.push_back(new QPushButton("Shaders", m_leftColumnContainer));
        QObject::connect(m_configButtons[0], &QPushButton::released, [this]{ this->HandleConfigAreaChange(0); });
        m_configButtons.push_back(new QPushButton("Vertex Input", m_leftColumnContainer));
        QObject::connect(m_configButtons[1], &QPushButton::released, [this]{ this->HandleConfigAreaChange(1); });
        m_configButtons.push_back(new QPushButton("Viewport", m_leftColumnContainer));
        QObject::connect(m_configButtons[2], &QPushButton::released, [this]{ this->HandleConfigAreaChange(2); });
        m_configButtons.push_back(new QPushButton("Rasterizer", m_leftColumnContainer));
        QObject::connect(m_configButtons[3], &QPushButton::released, [this]{ this->HandleConfigAreaChange(3); });
        m_configButtons.push_back(new QPushButton("Multisampling", m_leftColumnContainer));
        QObject::connect(m_configButtons[4], &QPushButton::released, [this]{ this->HandleConfigAreaChange(4); });
        m_configButtons.push_back(new QPushButton("Depth and Stencil", m_leftColumnContainer));
        QObject::connect(m_configButtons[5], &QPushButton::released, [this]{ this->HandleConfigAreaChange(5); });
        m_configButtons.push_back(new QPushButton("Render Pass State", m_leftColumnContainer));
        QObject::connect(m_configButtons[6], &QPushButton::released, [this]{ this->HandleConfigAreaChange(6); });
        m_configButtons.push_back(new QPushButton("Descriptors", m_leftColumnContainer));
        QObject::connect(m_configButtons[7], &QPushButton::released, [this]{ this->HandleConfigAreaChange(7); });
        m_descriptorBlockIdx = 7;
        for (QPushButton* button : m_configButtons) {
            m_leftColumnContainer->layout()->addWidget(button);
        }
    }

    void MainWindow::AddConfigBlocks() {
        QWidget* shaderWidget = new QWidget(m_rightTopContainer);
        shaderWidget->setLayout(new QVBoxLayout(shaderWidget));
        m_configBlocks.push_back(shaderWidget);
        MakeShaderBlock(shaderWidget, "Vertex");
        MakeShaderBlock(shaderWidget, "Fragment");
        MakeShaderBlock(shaderWidget, "Tess Control");
        MakeShaderBlock(shaderWidget, "Tess Eval");
        MakeShaderBlock(shaderWidget, "Geometry");

        m_configBlocks.push_back(MakeVertexInputBlock());
        m_configBlocks.push_back(MakeViewportStateBlock());
        m_configBlocks.push_back(MakeRasterizerBlock());
        m_configBlocks.push_back(MakeMultisampleBlock());
        m_configBlocks.push_back(MakeDepthStencilBlock());
        m_configBlocks.push_back(MakeRenderPassBlock());
        m_configBlocks.push_back(nullptr);
        MakeDescriptorBlock();

        for (QWidget* widget : m_configBlocks) {
            widget->hide();
        }
    }

    void MainWindow::HandleConfigAreaChange(int toIdx) {
        m_configBlocks[m_unhiddenIdx]->hide();
        m_configBlocks[toIdx]->show();
        m_unhiddenIdx = toIdx;
    }

    void MainWindow::HandleShaderFileDialog(QLineEdit* field) {
        field->setText(QFileDialog::getOpenFileName(this, tr("Open File"), ".", tr("Shader Files (*.spv)")));
        // TODO change shaders
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

    void MainWindow::MakeShaderBlock(QWidget* parent, QString labelStr) {
        QWidget* container = new QWidget(parent);

        QLabel* label = new QLabel(labelStr, container);

        QLineEdit* field = new QLineEdit("", container);
        field->setReadOnly(true);

        QPushButton* dialogBtn = new QPushButton(container);
        dialogBtn->setIcon(QIcon(IMAGEDIR"file_dialog.svg"));

        QHBoxLayout* layout = new QHBoxLayout(container);
        layout->addWidget(label);
        layout->addWidget(field);
        layout->addWidget(dialogBtn);

        container->setLayout(layout);
        parent->layout()->addWidget(container);

        QObject::connect(dialogBtn, &QPushButton::released, [this, field]{ this->HandleShaderFileDialog(field); });
    }

    QWidget* MainWindow::MakeVertexInputBlock() {
        QWidget* container = new QWidget(m_rightTopContainer);
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
        spinBox->setRange(0, 32); // TODO int(m_vulkan->Limits().maxTessellationPatchSize)
        QObject::connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
           HandleConfigValueChange<uint32_t>(Config().writables.patchControlPoints, ReloadFlags::Pipeline, value);
        });

        return container;
    }

    QWidget* MainWindow::MakeViewportStateBlock() {
        // ----------- Viewport -------------
        QWidget* container = new QWidget(m_rightTopContainer);
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
        QWidget* container = new QWidget(m_rightTopContainer);
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
        QWidget* container = new QWidget(m_rightTopContainer);
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
        QWidget* container = new QWidget(m_rightTopContainer);

        // ----------- Depth Test -------------
        QLabel* testLabel = new QLabel("Test Enable");
        QComboBox* testBox = MakeComboBox(container, {"False", "True"});
        QObject::connect(testBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.depthTestEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Depth Write -------------
        QLabel* writeLabel = new QLabel("Write Enable");
        QComboBox* writeBox = MakeComboBox(container, {"False", "True"});
        QObject::connect(writeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.depthWriteEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Bounds Enable -------------
        QLabel* boundsLabel = new QLabel("Bounds Enable");
        QComboBox* boundsBox = MakeComboBox(container, {"False", "True"});
        QObject::connect(boundsBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.depthBoundsTest, ReloadFlags::Pipeline, index);
        });

        // ----------- Stencil Test -------------
        QLabel* stencilLabel = new QLabel("Stencil Test Enable");
        QComboBox* stencilBox = MakeComboBox(container, {"False", "True"});
        QObject::connect(stencilBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkBool32>(Config().writables.stencilTestEnable, ReloadFlags::Pipeline, index);
        });

        // ----------- Compare Op -------------
        QLabel* compareLabel = new QLabel("Compare Op");
        QComboBox* compareBox = MakeComboBox(container, { "Never", "Less", "Equal",
                "Less or Equal", "Greater", "Not Equal", "Greater or Equal", "Always"
        });
        QObject::connect(compareBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
            HandleConfigValueChange<VkCompareOp>(Config().writables.depthCompareOp, ReloadFlags::Pipeline, index);
        });

        QGridLayout* layout = new QGridLayout(container);
        container->setLayout(layout);

        layout->addWidget(testLabel, 0, 0);
        layout->addWidget(testBox, 1, 0);
        layout->addWidget(writeLabel, 0, 1);
        layout->addWidget(writeBox, 1, 1);
        layout->addWidget(boundsLabel, 2, 0);
        layout->addWidget(boundsBox, 3, 0);
        layout->addWidget(stencilLabel, 2, 1);
        layout->addWidget(stencilBox, 3, 1);
        layout->addWidget(compareLabel, 4, 0);
        layout->addWidget(compareBox, 5, 0);

        return container;
    }

    QWidget* MainWindow::MakeRenderPassBlock() {
        QWidget* container = new QWidget(m_rightTopContainer);
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
        QWidget*& container = m_configBlocks[m_descriptorBlockIdx];
        if (container) delete container;
        container = new QWidget(m_rightTopContainer);
        QGridLayout* layout = new QGridLayout(container);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 4);
        layout->setAlignment(Qt::AlignTop);
        container->setLayout(layout);

        QWidget* leftGroup = new QWidget(container);
        QWidget* rightGroup = new QWidget(container);
        leftGroup->setLayout(new QVBoxLayout(leftGroup));
        rightGroup->setLayout(new QVBoxLayout(rightGroup));
        layout->addWidget(leftGroup, 0, 0);
        layout->addWidget(rightGroup, 0, 1);
        leftGroup->layout()->setAlignment(Qt::AlignTop);
        rightGroup->layout()->setAlignment(Qt::AlignTop);

        if (m_vulkan) {
            Descriptors* descriptors = m_vulkan->GetDescriptors();
            if (descriptors) {
                QVector<QWidget*> spvWidgets;
                for (auto& set : descriptors->Buffers().keys()) {
                    for (int i = 0; i < descriptors->Buffers()[set].size(); ++i) {
                        SpvResourceWidget* spvWidget = new SpvResourceWidget(descriptors,
                            descriptors->Buffers()[set][i].descriptor.resource, set, i, rightGroup);
                        spvWidgets.push_back(spvWidget);
                        rightGroup->layout()->addWidget(spvWidget);
                    }
                }
                for (auto& set : descriptors->Buffers().keys()) {
                    for (int i = 0; i < descriptors->Images()[set].size(); ++i) {
                        SpvResourceWidget* spvWidget = new SpvResourceWidget(descriptors,
                            descriptors->Images()[set][i].descriptor.resource, set, i, rightGroup);
                        spvWidgets.push_back(spvWidget);
                        rightGroup->layout()->addWidget(spvWidget);
                    }
                }
                for (auto& pushConstant : descriptors->PushConstants().values()) {
                    SpvResourceWidget* spvWidget = new SpvResourceWidget(descriptors,
                        pushConstant.resource, 0, 0, rightGroup);
                    spvWidgets.push_back(spvWidget);
                    rightGroup->layout()->addWidget(spvWidget);
                }
                for (int i = 0; i < spvWidgets.size(); ++i) {
                    QPushButton* btn = new QPushButton(reinterpret_cast<SpvResourceWidget*>(spvWidgets[i])->Title(), leftGroup);
                    leftGroup->layout()->addWidget(btn);
                    QObject::connect(btn, &QPushButton::released, [this, spvWidgets, i]{
                        spvWidgets[this->m_activeDescriptorIdx]->hide();
                        spvWidgets[i]->show();
                        this->m_activeDescriptorIdx = i;
                    });
                    spvWidgets[i]->hide();
                }
                spvWidgets[0]->show();
                m_activeDescriptorIdx = 0;
            }
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
