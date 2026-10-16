#include <gtest/gtest.h>

#include <QLineEdit>
#include <QTreeWidget>

#include "robomongo/gui/widgets/explorer/ExplorerTreeReveal.h"
#include "robomongo-unit-tests/QtTestApplication.h"

using namespace Robomongo;

namespace
{
    // server > database > "Collections" folder > 40 collections, all collapsed
    struct ExplorerLikeTree
    {
        QTreeWidget tree;
        QTreeWidgetItem *server;
        QTreeWidgetItem *database;
        QTreeWidgetItem *folder;
        std::vector<QTreeWidgetItem *> collections;
        int expandedSignals = 0;
        int currentChangedSignals = 0;

        ExplorerLikeTree()
        {
            tree.resize(240, 160);
            server = new QTreeWidgetItem(&tree, QStringList("localhost"));
            database = new QTreeWidgetItem(server, QStringList("shop"));
            folder = new QTreeWidgetItem(database, QStringList("Collections"));
            for (int i = 0; i < 40; ++i)
                collections.push_back(new QTreeWidgetItem(folder, QStringList(QString("c%1").arg(i))));

            QObject::connect(&tree, &QTreeWidget::itemExpanded,
                             [this](QTreeWidgetItem *) { ++expandedSignals; });
            QObject::connect(&tree, &QTreeWidget::currentItemChanged,
                             [this](QTreeWidgetItem *, QTreeWidgetItem *) { ++currentChangedSignals; });
        }

        bool isInView(QTreeWidgetItem *item) const
        {
            QRect const rect = tree.visualItemRect(item);
            return rect.isValid() && tree.viewport()->rect().contains(rect);
        }
    };
}

class ExplorerTreeRevealTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
};

TEST_F(ExplorerTreeRevealTests, selectsItem)
{
    ExplorerLikeTree t;

    revealExplorerItem(&t.tree, t.collections[30]);

    EXPECT_EQ(t.collections[30], t.tree.currentItem());
    EXPECT_TRUE(t.collections[30]->isSelected());
}

TEST_F(ExplorerTreeRevealTests, expandsCollapsedAncestors)
{
    ExplorerLikeTree t;

    revealExplorerItem(&t.tree, t.collections[30]);

    EXPECT_TRUE(t.server->isExpanded());
    EXPECT_TRUE(t.database->isExpanded());
    EXPECT_TRUE(t.folder->isExpanded());
    EXPECT_FALSE(t.collections[30]->isExpanded());
}

TEST_F(ExplorerTreeRevealTests, doesNotTriggerExplorerReloads)
{
    // Explorer folders reload and recreate their children on itemExpanded
    ExplorerLikeTree t;

    revealExplorerItem(&t.tree, t.collections[30]);

    EXPECT_EQ(0, t.expandedSignals);
    EXPECT_EQ(0, t.currentChangedSignals);
}

TEST_F(ExplorerTreeRevealTests, signalsWorkAgainAfterReveal)
{
    ExplorerLikeTree t;
    revealExplorerItem(&t.tree, t.collections[30]);

    t.collections[30]->addChild(new QTreeWidgetItem(QStringList("Indexes")));
    t.collections[30]->setExpanded(true);

    EXPECT_EQ(1, t.expandedSignals);
}

TEST_F(ExplorerTreeRevealTests, scrollsItemIntoView)
{
    ExplorerLikeTree t;
    t.tree.show();
    t.server->setExpanded(true);
    t.database->setExpanded(true);
    t.folder->setExpanded(true);
    t.tree.scrollToTop();
    ASSERT_FALSE(t.isInView(t.collections[35]));

    revealExplorerItem(&t.tree, t.collections[35]);

    EXPECT_TRUE(t.isInView(t.collections[35]));
}

TEST_F(ExplorerTreeRevealTests, leavesKeyboardFocusAlone)
{
    QWidget window;
    QLineEdit editor(&window);
    ExplorerLikeTree t;
    t.tree.setParent(&window);
    window.show();
    editor.setFocus();
    QApplication::processEvents();
    QWidget *const focusedBefore = QApplication::focusWidget();

    revealExplorerItem(&t.tree, t.collections[30]);
    QApplication::processEvents();

    EXPECT_EQ(focusedBefore, QApplication::focusWidget());
    EXPECT_FALSE(t.tree.hasFocus());
    t.tree.setParent(nullptr);
}

TEST_F(ExplorerTreeRevealTests, ignoresItemFromAnotherTree)
{
    ExplorerLikeTree t;
    ExplorerLikeTree other;
    t.tree.setCurrentItem(t.server);

    revealExplorerItem(&t.tree, other.collections[5]);

    EXPECT_EQ(t.server, t.tree.currentItem());
    EXPECT_FALSE(other.folder->isExpanded());
}

TEST_F(ExplorerTreeRevealTests, ignoresNullArguments)
{
    ExplorerLikeTree t;
    t.tree.setCurrentItem(t.server);

    revealExplorerItem(&t.tree, nullptr);
    revealExplorerItem(nullptr, t.collections[0]);

    EXPECT_EQ(t.server, t.tree.currentItem());
    EXPECT_FALSE(t.folder->isExpanded());
}
