#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PipelineConfig.h"

#include <QLineEdit>

#include "PipelineConfig.h"

using namespace vpa;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_ui(new Ui::MainWindow), m_unhiddenIdx(0) {
    m_ui->setupUi(this);
    m_layout = new QGridLayout();
    m_leftColumnContainer = new QWidget(this);
    m_rightTopContainer = new QWidget(this);
    m_rightBottomContainer = new QWidget(this);
    m_layout->addWidget(m_leftColumnContainer, 0, 0);
    m_layout->addWidget(m_rightTopContainer, 1, 0);
    m_layout->addWidget(m_rightBottomContainer, 1, 1);
    m_leftColumnContainer->setLayout(new QVBoxLayout(m_leftColumnContainer));
    m_leftColumnContainer->layout()->setAlignment(Qt::AlignTop);
    m_leftColumnContainer->setGeometry(10, 20, this->width() / 4, this->height() / 2);
    m_rightTopContainer->setGeometry(this->width() / 4, 20, (3 * this->width()) / 4, this->height() / 2);
    m_rightBottomContainer->setLayout(new QVBoxLayout(m_rightBottomContainer));
    m_rightBottomContainer->setGeometry(this->width() / 4, this->height() / 2, (3 * this->width()) / 4, this->height() / 2);
    AddConfigButtons();
    AddConfigBlocks();

    m_cacheBtn = new QPushButton("Create cache", m_leftColumnContainer);
    m_leftColumnContainer->layout()->addWidget(m_cacheBtn);
    QObject::connect(m_cacheBtn, &QPushButton::released, [this]{
        this->m_vulkan->WritePipelineCache();
    });

    m_vulkan = new VulkanMain(m_rightBottomContainer);
}

MainWindow::~MainWindow() {
    delete m_ui;
}

void MainWindow::AddConfigButtons() {
    // Shaders
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
    for (QPushButton* button : m_configButtons) {
        m_leftColumnContainer->layout()->addWidget(button);
    }
}

void MainWindow::AddConfigBlocks() {
    // ------ Shaders ------------
    QWidget* shaderWidget = new QWidget(m_rightTopContainer);
    shaderWidget->setLayout(new QVBoxLayout(shaderWidget));
    m_configBlocks.push_back(shaderWidget);
    MakeShaderBlock(shaderWidget, "Vertex");
    MakeShaderBlock(shaderWidget, "Fragment");
    MakeShaderBlock(shaderWidget, "Tess Control");
    MakeShaderBlock(shaderWidget, "Tess Eval");
    MakeShaderBlock(shaderWidget, "Geometry");

    // ------ Vertex Input -------
    m_configBlocks.push_back(MakeVertexInputBlock());
    m_configBlocks.push_back(MakeViewportStateBlock());
    m_configBlocks.push_back(MakeRasterizerBlock());
    m_configBlocks.push_back(MakeMultisampleBlock());
    m_configBlocks.push_back(MakeDepthStencilBlock());
    m_configBlocks.push_back(MakeRenderPassBlock());

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
    field->setText(QFileDialog::getOpenFileName(this,
        tr("Open File"), ".", tr("Shader Files (*.spv)")));
}

//TODO allow multiple viewports
void MainWindow::HandleViewChangeApply(QVector<QLineEdit*> v) {
    PipelineConfig& config = this->m_vulkan->GetConfig();
    VkViewport& viewport = config.viewports[0];
    viewport.x = float(std::atof(v.at(0)->text().toStdString().c_str()));
    viewport.y = float(std::atof(v.at(1)->text().toStdString().c_str()));
    viewport.width = float(std::atof(v.at(2)->text().toStdString().c_str()));
    viewport.height = float(std::atof(v.at(3)->text().toStdString().c_str()));
    viewport.minDepth = float(std::atof(v.at(4)->text().toStdString().c_str()));
    viewport.maxDepth = float(std::atof(v.at(5)->text().toStdString().c_str()));
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

// TODO expandable vertex inputs (max 16, although track for multiple such as with mat4)
QWidget* MainWindow::MakeVertexInputBlock() {
    std::map<QString, VkPrimitiveTopology> topologies;
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Point List", VK_PRIMITIVE_TOPOLOGY_POINT_LIST));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Line List", VK_PRIMITIVE_TOPOLOGY_LINE_LIST));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Line Strip", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Triangle List", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Triangle Strip", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Triangle Fan", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Line List With Adjacency", VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Line Strip With Adjacency", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Triangle List With Adjacency", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Triangle Strip With Adjacency", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY));
    topologies.insert(std::pair<QString, VkPrimitiveTopology>("Patch List", VK_PRIMITIVE_TOPOLOGY_PATCH_LIST));

    QWidget* parent = new QWidget(m_rightTopContainer);
    QLabel* topologyLabel = new QLabel("Topology", parent);
    QComboBox* topologyBox = MakeComboBox(parent, {});
    std::map<QString, VkPrimitiveTopology>::iterator it;
    for (it = topologies.begin(); it != topologies.end(); it++) {
        topologyBox->addItem(it->first);
    }

    QObject::connect(topologyBox, (void(QComboBox::*)(int))(&QComboBox::currentIndexChanged), [this, topologyBox, topologies](int index) {
        QString value = topologyBox->currentText();
        PipelineConfig& config = this->m_vulkan->GetConfig();
        config.writablePipelineConfig.topology = topologies.at(value);
        this->m_vulkan->WritePipelineConfig();
        this->m_vulkan->Reload(ReloadFlags::EVERYTHING);
    });

    QLabel* primRestartLabel = new QLabel("Primitive Restart", parent);
    QComboBox* primRestartBox = MakeComboBox(parent, { "False", "True" });
    QObject::connect(primRestartBox, (void(QComboBox::*)(int))(&QComboBox::currentIndexChanged), [this, primRestartBox](int index){
        QString text = primRestartBox->currentText();
        bool value = text == "True" ? true : false;
        PipelineConfig& config = this->m_vulkan->GetConfig();
        config.writablePipelineConfig.primitiveRestartEnable = value;
        this->m_vulkan->WritePipelineConfig();
        this->m_vulkan->Reload(ReloadFlags::EVERYTHING);
    });

    QLabel* patchPointsLabel = new QLabel("Patch Point Count", parent);
    QLineEdit* patchPointsBox = new QLineEdit("0", parent);
    patchPointsBox->setValidator(new QIntValidator(0, 9, this));
    QObject::connect(patchPointsBox, (void(QLineEdit::*)(int))(&QLineEdit::textChanged), [this, patchPointsBox](int index) {
       QString text = patchPointsBox->text();
       uint32_t value = uint32_t(std::atoi(text.toStdString().c_str()));
       PipelineConfig& config = this->m_vulkan->GetConfig();
       config.writablePipelineConfig.patchControlPoints = value;
       this->m_vulkan->WritePipelineConfig();
       this->m_vulkan->Reload(ReloadFlags::EVERYTHING);
    });

    QGridLayout* layout = new QGridLayout(parent);
    parent->setLayout(layout);

    layout->addWidget(topologyLabel, 0, 0);
    layout->addWidget(topologyBox, 1, 0);
    layout->addWidget(primRestartLabel, 0, 1);
    layout->addWidget(primRestartBox, 1, 1);
    layout->addWidget(patchPointsLabel, 0, 2);
    layout->addWidget(patchPointsBox, 1, 2);

    return parent;
}

// TODO add supoort for multiple viewports and scissors
QWidget* MainWindow::MakeViewportStateBlock() {
    // Viewport
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

    // Scissor
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
    QLabel* polygonModeLabel = new QLabel("Polygon Mode");
    QComboBox* polygonModeBox = MakeComboBox(container, {"Fill", "Line", "Point"});

    QLabel* rasterizerDiscardLabel = new QLabel("Discard Enable");
    QComboBox* rasterizerDiscardBox = MakeComboBox(container, {"False", "True"});

    QLabel* lineWidthLabel = new QLabel("Line Width");
    QLineEdit* lineWidthBox = new QLineEdit("1.0", container);

    QLabel* cullModeLabel = new QLabel("Cull Mode");
    QComboBox* cullModeBox = MakeComboBox(container, {"None", "Front", "Back", "Front and Back"});

    QLabel* frontFaceLabel = new QLabel("Front Face");
    QComboBox* frontFaceBox = MakeComboBox(container, {"Counter Clockwise", "Clockwise"});

    QLabel* depthClampLabel = new QLabel("Depth Clamp Enable");
    QComboBox* depthClampBox = MakeComboBox(container, {"False", "True"});

    QLabel* depthBiasLabel = new QLabel("Depth Bias Enable");
    QComboBox* depthBiasBox = MakeComboBox(container, {"False", "True"});

    QLabel* depthBiasConstLabel = new QLabel("Depth Bias Constant");
    QLineEdit* depthBiasConstBox = new QLineEdit("1.0", container);
    depthBiasConstBox->setValidator(new QDoubleValidator(0.0, 9.0, 3, container));

    QLabel* depthBiasClampLabel = new QLabel("Depth Bias Clamp");
    QLineEdit* depthBiasClampBox = new QLineEdit("1.0", container);
    depthBiasConstBox->setValidator(new QDoubleValidator(0.0, 9.0, 3, container));

    QLabel* depthBiasSlopeLabel = new QLabel("Depth Bias Slope");
    QLineEdit* depthBiasSlopeBox = new QLineEdit("1.0", container);
    depthBiasConstBox->setValidator(new QDoubleValidator(0.0, 9.0, 3, container));

    QGridLayout* layout = new QGridLayout(container);
    container->setLayout(layout);

    layout->addWidget(polygonModeLabel, 0, 0);
    layout->addWidget(polygonModeBox, 1, 0);
    layout->addWidget(rasterizerDiscardLabel, 0, 1);
    layout->addWidget(rasterizerDiscardBox, 1, 1);
    layout->addWidget(lineWidthLabel, 0, 2);
    layout->addWidget(lineWidthBox, 1, 2);
    layout->addWidget(cullModeLabel, 0, 3);
    layout->addWidget(cullModeBox, 1, 3);
    layout->addWidget(frontFaceLabel, 0, 4);
    layout->addWidget(frontFaceBox, 1, 4);

    layout->addWidget(depthClampLabel, 2, 0);
    layout->addWidget(depthClampBox, 3, 0);
    layout->addWidget(depthBiasLabel, 2, 1);
    layout->addWidget(depthBiasBox, 3, 1);
    layout->addWidget(depthBiasConstLabel, 2, 2);
    layout->addWidget(depthBiasConstBox, 3, 2);
    layout->addWidget(depthBiasClampLabel, 2, 3);
    layout->addWidget(depthBiasClampBox, 3, 3);
    layout->addWidget(depthBiasSlopeLabel, 2, 4);
    layout->addWidget(depthBiasSlopeBox, 3, 4);

    return container;
}

QWidget* MainWindow::MakeMultisampleBlock() {
    QWidget* container = new QWidget(m_rightTopContainer);

    QLabel* sampleCountLabel = new QLabel("Sample Count", container);
    QComboBox* sampleCountBox = MakeComboBox(container, {"1", "2", "4", "8", "16", "32", "64"});

    QLabel* minShadingLabel = new QLabel("Min Sample Shading", container);
    QLineEdit* minShadingBox = new QLineEdit("0.2", container);
    minShadingBox->setValidator(new QDoubleValidator(0.0, 1.0, 3, container));

    QGridLayout* layout = new QGridLayout(container);
    container->setLayout(layout);

    layout->addWidget(sampleCountLabel, 0, 0);
    layout->addWidget(sampleCountBox, 1, 0);
    layout->addWidget(minShadingLabel, 0, 1);
    layout->addWidget(minShadingBox, 1, 1);

    return container;
}

QWidget* MainWindow::MakeDepthStencilBlock() {
    QWidget* container = new QWidget(m_rightTopContainer);

    QLabel* testLabel = new QLabel("Test Enable");
    QComboBox* testBox = MakeComboBox(container, {"False", "True"});

    QLabel* writeLabel = new QLabel("Write Enable");
    QComboBox* writeBox = MakeComboBox(container, {"False", "True"});

    QLabel* boundsLabel = new QLabel("Bounds Enable");
    QComboBox* boundsBox = MakeComboBox(container, {"False", "True"});

    QLabel* stencilLabel = new QLabel("Stencil Test Enable");
    QComboBox* stencilBox = MakeComboBox(container, {"False", "True"});

    QLabel* compareLabel = new QLabel("Compare Op");
    QComboBox* compareBox = MakeComboBox(container, {
        "Never", "Less", "Equal", "Less or Equal", "Greater",
        "Not Equal", "Greater or Equal", "Always"
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

QComboBox* MainWindow::MakeComboBox(QWidget* parent, QVector<QString> items) {
    QComboBox* box = new QComboBox(parent);
    for (QString& str : items) {
        box->addItem(str);
    }
    return box;
}
