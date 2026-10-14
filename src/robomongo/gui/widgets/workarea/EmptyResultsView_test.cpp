#include <gtest/gtest.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMenu>

#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/EmptyResultsView.h"
#include "robomongo-unit-tests/QtTestApplication.h"

using namespace Robomongo;

namespace
{
    QStringList actionTexts(const QMenu &menu)
    {
        QStringList texts;
        for (QAction *action : menu.actions())
            texts << (action->isSeparator() ? QString("---") : action->text());
        return texts;
    }

    QAction *actionNamed(const QMenu &menu, const QString &text)
    {
        for (QAction *action : menu.actions()) {
            if (action->text() == text)
                return action;
        }
        return nullptr;
    }

    struct Fixture
    {
        FindFrame *output = new FindFrame(nullptr);
        EmptyResultsView view { output };
        QAction insert { "Insert Document...", nullptr };

        Fixture()
        {
            output->sciScintilla()->setText("Fetched 0 record(s)");
            output->sciScintilla()->setReadOnly(true);
        }
    };
}

class EmptyResultsViewTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
};

TEST_F(EmptyResultsViewTests, menuStartsWithInsertDocument)
{
    Fixture f;
    f.view.setInsertDocumentAction(&f.insert);
    QMenu menu;

    f.view.populateContextMenu(&menu);

    EXPECT_EQ(QStringList({ "Insert Document...", "---", "Copy", "Select All" }), actionTexts(menu));
    EXPECT_EQ(&f.insert, menu.actions().first());
}

TEST_F(EmptyResultsViewTests, menuWithoutInsertActionKeepsTextActionsOnly)
{
    Fixture f;
    QMenu menu;

    f.view.populateContextMenu(&menu);

    EXPECT_EQ(QStringList({ "Copy", "Select All" }), actionTexts(menu));
}

TEST_F(EmptyResultsViewTests, insertDocumentTriggersSharedAction)
{
    Fixture f;
    int triggered = 0;
    QObject::connect(&f.insert, &QAction::triggered, [&triggered]() { ++triggered; });
    f.view.setInsertDocumentAction(&f.insert);
    QMenu menu;
    f.view.populateContextMenu(&menu);

    actionNamed(menu, "Insert Document...")->trigger();

    EXPECT_EQ(1, triggered);
}

TEST_F(EmptyResultsViewTests, copyIsDisabledWithoutSelection)
{
    Fixture f;
    QMenu menu;

    f.view.populateContextMenu(&menu);

    EXPECT_FALSE(actionNamed(menu, "Copy")->isEnabled());
}

TEST_F(EmptyResultsViewTests, selectAllSelectsOutputAndCopyCopiesIt)
{
    Fixture f;
    QMenu menu;
    f.view.populateContextMenu(&menu);

    actionNamed(menu, "Select All")->trigger();
    EXPECT_EQ(QString("Fetched 0 record(s)"), f.output->sciScintilla()->selectedText());

    QMenu afterSelection;
    f.view.populateContextMenu(&afterSelection);
    QAction *copy = actionNamed(afterSelection, "Copy");
    ASSERT_TRUE(copy->isEnabled());

    QApplication::clipboard()->clear();
    copy->trigger();
    EXPECT_EQ(QString("Fetched 0 record(s)"), QApplication::clipboard()->text());
}

TEST_F(EmptyResultsViewTests, rightClickUsesCustomMenu)
{
    Fixture f;

    EXPECT_EQ(Qt::CustomContextMenu, f.output->sciScintilla()->contextMenuPolicy());
    EXPECT_EQ(&f.view, f.output->parentWidget());
}

TEST_F(EmptyResultsViewTests, hasNoSelectionForNotifier)
{
    Fixture f;
    INotifierObserver const &observer = f.view;

    EXPECT_FALSE(observer.selectedIndex().isValid());
    EXPECT_TRUE(observer.selectedIndexes().isEmpty());
}
