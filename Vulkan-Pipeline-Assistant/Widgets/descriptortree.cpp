#include "descriptortree.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>

#include "../Vulkan/descriptors.h"
#include "../Vulkan/spirvresource.h"
#include "containerwidget.h"
#include "spvmatrixwidget.h"
#include "spvvectorwidget.h"
#include "spvarraywidget.h"
#include "spvstructwidget.h"
#include "spvimagewidget.h"

namespace vpa {
    const QString VecStrings[4] = { "float", "vec2", "vec3", "vec4" };

    DescriptorNodeRoot::~DescriptorNodeRoot() {
        delete child;
    }

    void DescriptorNodeRoot::WriteDescriptorData() {
        tree->WriteDescriptorData(this);
    }
    void DescriptorNodeRoot::WriteDescriptorData(QString fileName) {
        tree->WriteDescriptorData(this, fileName);
    }

    DescriptorNodeLeaf::~DescriptorNodeLeaf() {
        root->tree->m_descriptorNodes[treeItem] = nullptr;
        delete widget;
        for (DescriptorNodeLeaf* child : children) {
            delete child;
        }
    }

    void DescriptorNodeLeaf::Data(unsigned char* dataPtr) const {
        if (type->Type() == SpvTypeName::Struct) {
            size_t offset = 0;
            for (int i = 0; i < children.size(); ++i) {
                offset = reinterpret_cast<SpvStructType*>(type)->memberOffsets[i];
                children[i]->Data(dataPtr + offset);
            }
        }
        else {
            widget->Data(dataPtr);
        }
    }

    void DescriptorNodeLeaf::Fill(const unsigned char* data) {
        if (type->Type() == SpvTypeName::Struct) {
            size_t offset = 0;
            for (int i = 0; i < children.size(); ++i) {
                offset = reinterpret_cast<SpvStructType*>(type)->memberOffsets[i];
                children[i]->Fill(data + offset);
            }
        }
        else {
            widget->Fill(data);
        }
    }

    DescriptorTree::~DescriptorTree() {
        for (QTreeWidgetItem* nodeKey : m_descriptorNodes.keys()) {
            DescriptorNode* node = m_descriptorNodes[nodeKey];
            if (node != nullptr && node->Type() == NodeType::Root) {
                delete node; // Only deleting root since it recurseively deletes all nodes, even those normally hidden
            }

            m_tree->removeItemWidget(nodeKey, 0);
            for(QTreeWidgetItem* item : nodeKey->takeChildren()) {
                nodeKey->removeChild(item);
            }
            delete nodeKey;
        }
    }

    void DescriptorTree::WriteDescriptorData(DescriptorNodeRoot* root) {
        if (!root->child) return;
        if (root->resource->group->Group() == SpvGroupName::PushConstant) {
            unsigned char* dataPtr = m_descriptors->PushConstantData(reinterpret_cast<SpvPushConstantGroup*>(root->resource->group)->stage);
            if (dataPtr != nullptr) {
                root->child->Data(dataPtr);
                m_descriptors->CompletePushConstantData();
            }
        }
        else {
            unsigned char* dataPtr = m_descriptors->MapBufferPointer(root->descriptorSet, root->descriptorIndex);
            if (dataPtr != nullptr) {
                root->child->Data(dataPtr);
                m_descriptors->UnmapBufferPointer(root->descriptorSet, root->descriptorIndex);
            }
        }
    }

    void DescriptorTree::WriteDescriptorData(DescriptorNodeRoot* root, QString fileName) {
        assert(root->resource->group->Group() == SpvGroupName::Image);
        m_descriptors->LoadImage(root->descriptorSet, root->descriptorIndex, fileName);
    }

    QString DescriptorTree::MakeGroupInfoText(DescriptorNodeRoot& root) {
        if (root.resource->group->Group() != SpvGroupName::PushConstant) {
        return "layout(set = " + QString::number(root.descriptorSet) + ", binding = " + QString::number(static_cast<SpvDescriptorGroup*>(root.resource->group)->binding) + ") " +
                          SpvGroupNameStrings[size_t(root.resource->group->Group())] + " " + root.resource->name;
        }
        else {
            return ShaderStageStrings[size_t(reinterpret_cast<SpvPushConstantGroup*>(root.resource->group)->stage)] + " " +
                    SpvGroupNameStrings[size_t(root.resource->group->Group())] + ", " + root.resource->name;
        }
    }

    QString DescriptorTree::MakeTypeInfoText(SpvType* type) {
        if (type->Type() == SpvTypeName::Vector) {
            return VecStrings[(reinterpret_cast<SpvVectorType*>(type)->length - 1)];
        }
        else if (type->Type() == SpvTypeName::Matrix) {
            SpvMatrixType* mtype = reinterpret_cast<SpvMatrixType*>(type);
            return QStringLiteral("mat %1x%2").arg(mtype->rows).arg(mtype->columns);
        }
        else if (type->Type() == SpvTypeName::Image) {
            return "image " + SpvImageTypeNameStrings[size_t(reinterpret_cast<SpvImageType*>(type)->imageTypename)];
        }
        return "";
    }

    void DescriptorTree::HandleButtonClick(QTreeWidgetItem* item, int column) {
        Q_UNUSED(column)
        qDebug("Handling tree click");
        DescriptorNode* node = m_descriptorNodes[item];
        if (node->Type() == NodeType::Leaf) {
            DescriptorNodeLeaf* leaf = reinterpret_cast<DescriptorNodeLeaf*>(node);
            m_groupInfo->setText(MakeGroupInfoText(*leaf->root));
            m_typeInfo->setText(MakeTypeInfoText(leaf->type));
            m_typeWidget->ShowWidget(leaf->widget);
        }
        else {
            DescriptorNodeRoot* root = reinterpret_cast<DescriptorNodeRoot*>(node);
            m_groupInfo->setText(MakeGroupInfoText(*root));
            if (root->resource->type->Type() == SpvTypeName::Image || root->child->children.size() == 1) {
                m_typeInfo->setText(MakeTypeInfoText(root->child->type));
                m_typeWidget->ShowWidget(root->child->widget);
            }
            else {
                m_typeInfo->setText("");
                m_typeWidget->Clear();
            }
        }
    }

    DescriptorNodeLeaf* DescriptorTree::CreateDescriptorWidgetLeaf(SpvType* type, DescriptorNodeRoot* root, QTreeWidgetItem* parentTreeItem, bool topLevel) {
        DescriptorNodeLeaf* leaf = new DescriptorNodeLeaf();
        leaf->root = root;
        leaf->type = type;

        leaf->treeItem = parentTreeItem;
        if (!topLevel) {
            leaf->treeItem = new QTreeWidgetItem();
            leaf->treeItem->setText(0, type->name);
            parentTreeItem->addChild(leaf->treeItem);
            m_descriptorNodes.insert(leaf->treeItem, leaf);
        }

        switch (type->Type()) {
        case SpvTypeName::Vector:
            leaf->widget = new SpvVectorWidget(reinterpret_cast<SpvVectorType*>(type), root);
            break;
        case SpvTypeName::Matrix:
            leaf->widget = new SpvMatrixWidget(reinterpret_cast<SpvMatrixType*>(type), root);
            break;
        case SpvTypeName::Struct:
            leaf->widget = new SpvStructWidget(reinterpret_cast<SpvStructType*>(type));
            for (int i = 0; i < reinterpret_cast<SpvStructType*>(type)->members.size(); ++i) {
                leaf->children.push_back(CreateDescriptorWidgetLeaf(reinterpret_cast<SpvStructType*>(type)->members[i], root, leaf->treeItem, false));
            }
            break;
        case SpvTypeName::Array:
            leaf->widget = new SpvArrayWidget(reinterpret_cast<SpvArrayType*>(type), root);
            //leaf->children = ; TODO
            break;
        case SpvTypeName::Image:
            leaf->widget = new SpvImageWidget(reinterpret_cast<SpvImageType*>(type), root);
            break;
        case SpvTypeName::Count_:
            qWarning("Unknown spv widget type.");
        }

        leaf->widget->InitData();

        return leaf;
    }

    void DescriptorTree::CreateDescriptorWidgetTree(uint32_t set, int index, SpvResource* res) {
        DescriptorNodeRoot* root = new DescriptorNodeRoot();
        root->descriptorSet = set;
        root->descriptorIndex = index;
        root->resource = res;
        root->tree = this;

        QTreeWidgetItem* treeItem = new QTreeWidgetItem();
        treeItem->setText(0, res->name);
        root->child = CreateDescriptorWidgetLeaf(res->type, root, treeItem, true);

        m_descriptorNodes.insert(treeItem, root);
        m_tree->addTopLevelItem(treeItem);

        if (res->type->Type() != SpvTypeName::Image) WriteDescriptorData(root);
    }
}
