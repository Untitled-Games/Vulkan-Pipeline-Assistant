#include "tabbedcontainerwidget.h"

#include <QLayout>
#include <QPushButton>

#include "containerwidget.h"

namespace vpa {
    TabbedContainerWidget::TabbedContainerWidget(TabLayoutDirection layoutDirection, int tabWidth, int tabHeight, QWidget* parent)
        : QWidget(parent), m_layoutDirection(layoutDirection), m_leftTabIdx(0), m_tabWidth(tabWidth), m_tabHeight(tabHeight) {
        m_tabArea = new QWidget(this);
        m_container = new ContainerWidget(this);

        if (layoutDirection == TabLayoutDirection::Vertical) {
            setLayout(new QVBoxLayout(this));
            m_tabArea->setLayout(new QHBoxLayout(m_tabArea));
            m_container->resize(width(), height() - tabHeight);
            m_tabArea->layout()->setAlignment(Qt::AlignmentFlag::AlignLeft);
            m_tabArea->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        }
        else {
            setLayout(new QHBoxLayout(this));
            m_tabArea->setLayout(new QVBoxLayout(m_tabArea));
            m_tabArea->resize(tabWidth, height());
            m_container->resize(width() - tabWidth, height());
            m_tabArea->layout()->setAlignment(Qt::AlignmentFlag::AlignTop);
            m_tabArea->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        }

        layout()->addWidget(m_tabArea);
        layout()->addWidget(m_container);

        ClearContainer();
        m_visibleTabCount = VisibleTabCount();

        m_tabLeftButton = new QPushButton(m_tabArea);
        m_tabRightButton = new QPushButton(m_tabArea);
        if (layoutDirection == TabLayoutDirection::Vertical) {
            m_tabLeftButton->setIcon(QIcon(IMAGEDIR"left_arrow.svg"));
            m_tabRightButton->setIcon(QIcon(IMAGEDIR"right_arrow.svg"));
            m_tabLeftButton->setMinimumSize(ArrowDim, tabHeight);
            m_tabRightButton->setMinimumSize(ArrowDim, tabHeight);
        }
        else {
            m_tabLeftButton->setIcon(QIcon(IMAGEDIR"up_arrow.svg"));
            m_tabRightButton->setIcon(QIcon(IMAGEDIR"down_arrow.svg"));
            m_tabLeftButton->setMinimumSize(tabWidth, ArrowDim);
            m_tabRightButton->setMinimumSize(tabWidth, ArrowDim);
        }
        m_tabLeftButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        m_tabRightButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        QObject::connect(m_tabLeftButton, &QPushButton::released, [this]{ ShiftTabsLeft(); });
        QObject::connect(m_tabRightButton, &QPushButton::released, [this]{ ShiftTabsRight(); });

        m_tabArea->layout()->addWidget(m_tabLeftButton);
        m_tabArea->layout()->addWidget(m_tabRightButton);
        m_tabRightButton->hide();
        m_tabLeftButton->hide();

        m_tabArea->setContentsMargins(0, 0, 0, 0);
        m_tabArea->layout()->setContentsMargins(0, 0, 0, 0);
        m_tabArea->layout()->setSpacing(0);
        setContentsMargins(0, 0, 0, 0);
        layout()->setContentsMargins(0, 0, 0, 0);
        layout()->setSpacing(0);
    }

    TabbedContainerWidget::~TabbedContainerWidget() {
        for (QWidget* tab : m_tabs) {
            delete tab;
        }
        for (QWidget* item : m_containerItems) {
            delete item;
        }
    }

    void TabbedContainerWidget::AddTab(QString tabName, QWidget* containerItem) {
        m_visibleTabCount = VisibleTabCount();
        m_containerItems.push_back(containerItem);

        QPushButton* tabButton = new QPushButton(tabName, m_tabArea);
        tabButton->setMinimumSize(m_tabWidth, m_tabHeight);
        tabButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        containerItem->hide();
        QObject::connect(tabButton, &QPushButton::released, [this, containerItem]{ m_container->ShowWidget(containerItem); });

        m_tabArea->layout()->addWidget(tabButton);
        m_tabs.push_back(tabButton);
        if (m_tabs.size() >= m_visibleTabCount) tabButton->hide();
    }

    void TabbedContainerWidget::ClearContainer() {
        m_container->Clear();
    }

    void TabbedContainerWidget::InitSize() {
        for (int i = 0; i < m_tabs.size(); ++i) {
            m_tabs[i]->hide();
        }

        m_visibleTabCount = VisibleTabCount();
        m_leftTabIdx = 0;

        if (m_visibleTabCount < m_tabs.size()) {
            m_tabRightButton->show();
            m_tabLeftButton->show();
        }

        int maxIdx = std::min(m_visibleTabCount, m_tabs.size());
        for (int i = 0; i < maxIdx; ++i) {
            m_tabs[i]->show();
        }
    }

    void TabbedContainerWidget::resizeEvent(QResizeEvent* event) {
        Q_UNUSED(event)

        int oldCount = m_visibleTabCount;
        m_visibleTabCount = VisibleTabCount();
        if (oldCount == m_visibleTabCount) {
            return; // No change
        }
        else if (m_visibleTabCount >= m_tabs.size()) {
            if (oldCount >= m_tabs.size()) return;
            // From low space to full space, add widgets and remove arrows
            m_leftTabIdx = 0;
            for (int i = 0; i < m_tabs.size(); ++i) {
                m_tabs[i]->show();
            }

            m_tabRightButton->hide();
            m_tabLeftButton->hide();
        }
        else {
            if (oldCount < m_visibleTabCount) {
                // Need to add tabs up to m_visibleTabCount (max size)
                int endIdx = oldCount + m_leftTabIdx;
                int amountRight = m_visibleTabCount + m_leftTabIdx;
                int amountLeft = amountRight - m_tabs.size();
                amountRight = amountLeft > 0 ? amountRight - amountLeft : amountRight;
                for (int i = endIdx; i < amountRight; ++i) {
                    m_tabs[i]->show();
                }
                for (int i = m_leftTabIdx; i >= std::max(m_leftTabIdx - amountLeft, 0); --i) {
                    m_tabs[i]->show();
                }
            }
            else {
                // Lost space, need to remove tabs, show arrows.
                int adjustedCount = std::min(oldCount, m_tabs.size() - 1);
                for (int i = adjustedCount + m_leftTabIdx; i >= m_visibleTabCount + m_leftTabIdx; --i) {
                    m_tabs[i]->hide();
                }

                m_tabRightButton->show();
                m_tabLeftButton->show();
            }
        }

        if (oldCount <= m_visibleTabCount || m_visibleTabCount >= m_tabs.size()) return;

        int adjustedCount = std::min(oldCount, m_tabs.size() - 1);
        for (int i = adjustedCount + m_leftTabIdx; i >= m_visibleTabCount + m_leftTabIdx; --i) {
            m_tabs[i]->hide();
        }
    }

    int TabbedContainerWidget::VisibleTabCount() {
        if (m_layoutDirection == TabLayoutDirection::Vertical) return (width() - (ArrowDim * 2)) / m_tabWidth;
        else return (height() - (ArrowDim * 2)) / m_tabHeight;
    }

    void TabbedContainerWidget::ShiftTabsLeft() {
        if (m_visibleTabCount > m_tabs.size() || m_leftTabIdx == 0) return;

        --m_leftTabIdx;
        int rightIdx = m_leftTabIdx + m_visibleTabCount;
        m_tabs[rightIdx]->hide();
        m_tabs[m_leftTabIdx]->show();
    }

    void TabbedContainerWidget::ShiftTabsRight() {
        int rightIdx = m_leftTabIdx + m_visibleTabCount;
        if (m_visibleTabCount > m_tabs.size() || rightIdx >= m_tabs.size()) return;

        m_tabs[m_leftTabIdx++]->hide();
        m_tabs[rightIdx]->show();
    }
}
