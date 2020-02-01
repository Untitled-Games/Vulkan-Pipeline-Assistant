#ifndef DESCRIPTORTREE_H
#define DESCRIPTORTREE_H

#include "../common.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>

class QTextEdit;

namespace vpa {
    struct SpvType;
    struct SpvResource;
    class SpvWidget;
    class ContainerWidget;
    class DescriptorTree;
    class Descriptors;

    enum class NodeType {
        Root, Leaf
    };

    struct DescriptorNodeLeaf;

    struct DescriptorNode {
        virtual ~DescriptorNode() { }
        virtual NodeType Type() const = 0;
    };

    struct DescriptorNodeRoot : public DescriptorNode {
        uint32_t descriptorSet;
        int descriptorIndex;
        SpvResource* resource;
        DescriptorNodeLeaf* child;
        DescriptorTree* tree;

        void WriteDescriptorData();
        void WriteDescriptorData(QString fileName);

        ~DescriptorNodeRoot();

        NodeType Type() const {
            return NodeType::Root;
        }
    };

    struct DescriptorNodeLeaf : public DescriptorNode {
        DescriptorNodeRoot* root;
        QTreeWidgetItem* treeItem;
        SpvType* type;
        SpvWidget* widget; // Vector or Matrix or Array or Struct or Image
        QVector<DescriptorNodeLeaf*> children;

        void Data(unsigned char* dataPtr) const;
        void Fill(const unsigned char* data);

        ~DescriptorNodeLeaf();

        NodeType Type() const {
            return NodeType::Leaf;
        }
    };

    class DescriptorTree {
        friend struct DescriptorNodeRoot;
        friend struct DescriptorNodeLeaf;
    public:
        DescriptorTree(QTreeWidget* tree, QTextEdit* groupInfoWidget, QTextEdit* typeInfoWidget, ContainerWidget* typeWidget, Descriptors* descriptors)
            : m_tree(tree), m_descriptors(descriptors), m_groupInfo(groupInfoWidget), m_typeInfo(typeInfoWidget), m_typeWidget(typeWidget) {
            m_clickConnection = QObject::connect(m_tree, QOverload<QTreeWidgetItem*, int>::of(&QTreeWidget::itemClicked), [this](QTreeWidgetItem* item, int col){ HandleButtonClick(item, col); });
        }
        ~DescriptorTree();

        QString MakeGroupInfoText(DescriptorNodeRoot& root);
        QString MakeTypeInfoText(SpvType* type);
        DescriptorNodeLeaf* CreateDescriptorWidgetLeaf(SpvType* type, DescriptorNodeRoot* root, QTreeWidgetItem* parentTreeItem, bool topLevel);
        void CreateDescriptorWidgetTree(uint32_t set, int index, SpvResource* res);

        void WriteDescriptorData(DescriptorNodeRoot* root);
        void WriteDescriptorData(DescriptorNodeRoot* root, QString fileName);

    private slots:
        void HandleButtonClick(QTreeWidgetItem* item, int column);

    private:
        QTreeWidget* m_tree;
        QHash<QTreeWidgetItem*, DescriptorNode*> m_descriptorNodes;
        Descriptors* m_descriptors;

        QTextEdit* m_groupInfo;
        QTextEdit* m_typeInfo;
        ContainerWidget* m_typeWidget;
        QMetaObject::Connection m_clickConnection;
    };
}

#endif // DESCRIPTORTREE_H
