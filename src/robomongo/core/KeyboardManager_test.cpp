#include <gtest/gtest.h>

#include <QKeyEvent>

#include "robomongo/core/KeyboardManager.h"

using namespace Robomongo;

// Note: Qt reports the macOS Command key as Qt::ControlModifier
namespace
{
    QKeyEvent key(int k, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        return QKeyEvent(QEvent::KeyPress, k, modifiers);
    }

    Qt::KeyboardModifiers const Ctrl = Qt::ControlModifier;
    Qt::KeyboardModifiers const CtrlShift = Qt::ControlModifier | Qt::ShiftModifier;
    Qt::KeyboardModifiers const CtrlAlt = Qt::ControlModifier | Qt::AltModifier;
}

TEST(KeyboardManagerTests, newTab)
{
    for (QKeyEvent event : { key(Qt::Key_T, Ctrl), key(Qt::Key_Return, CtrlShift), key(Qt::Key_Enter, CtrlShift) })
        EXPECT_TRUE(KeyboardManager::isNewTabShortcut(&event)) << event.key();

    for (QKeyEvent event : { key(Qt::Key_T), key(Qt::Key_T, CtrlShift), key(Qt::Key_Return, Ctrl) })
        EXPECT_FALSE(KeyboardManager::isNewTabShortcut(&event)) << event.key();
}

TEST(KeyboardManagerTests, duplicateTab)
{
    QKeyEvent yes = key(Qt::Key_T, CtrlShift);
    QKeyEvent noShift = key(Qt::Key_T, Ctrl);

    EXPECT_TRUE(KeyboardManager::isDuplicateTabShortcut(&yes));
    EXPECT_FALSE(KeyboardManager::isDuplicateTabShortcut(&noShift));
}

TEST(KeyboardManagerTests, executeScript)
{
    QKeyEvent ctrlReturn = key(Qt::Key_Return, Ctrl);
    QKeyEvent ctrlEnter = key(Qt::Key_Enter, Ctrl);
    QKeyEvent plainReturn = key(Qt::Key_Return);

    EXPECT_TRUE(KeyboardManager::isExecuteScriptShortcut(&ctrlReturn));
    EXPECT_TRUE(KeyboardManager::isExecuteScriptShortcut(&ctrlEnter));
    EXPECT_FALSE(KeyboardManager::isExecuteScriptShortcut(&plainReturn));
}

TEST(KeyboardManagerTests, autocompleteShowAndHide)
{
    QKeyEvent ctrlSpace = key(Qt::Key_Space, Ctrl);
    QKeyEvent space = key(Qt::Key_Space);
    QKeyEvent escape = key(Qt::Key_Escape);

    EXPECT_TRUE(KeyboardManager::isAutoCompleteShortcut(&ctrlSpace));
    EXPECT_FALSE(KeyboardManager::isAutoCompleteShortcut(&space));
    EXPECT_TRUE(KeyboardManager::isHideAutoCompleteShortcut(&escape));
    EXPECT_FALSE(KeyboardManager::isHideAutoCompleteShortcut(&space));
}

TEST(KeyboardManagerTests, tabNavigation)
{
    QKeyEvent next = key(Qt::Key_Right, CtrlAlt);
    QKeyEvent previous = key(Qt::Key_Left, CtrlAlt);
    QKeyEvent ctrlRight = key(Qt::Key_Right, Ctrl);

    EXPECT_TRUE(KeyboardManager::isNextTabShortcut(&next));
    EXPECT_FALSE(KeyboardManager::isNextTabShortcut(&previous));
    EXPECT_FALSE(KeyboardManager::isNextTabShortcut(&ctrlRight));
    EXPECT_TRUE(KeyboardManager::isPreviousTabShortcut(&previous));
    EXPECT_FALSE(KeyboardManager::isPreviousTabShortcut(&next));
}

TEST(KeyboardManagerTests, focusQueryLine)
{
    QKeyEvent f6 = key(Qt::Key_F6);
    QKeyEvent f5 = key(Qt::Key_F5);

    EXPECT_TRUE(KeyboardManager::isSetFocusOnQueryLineShortcut(&f6));
    EXPECT_FALSE(KeyboardManager::isSetFocusOnQueryLineShortcut(&f5));
}

TEST(KeyboardManagerTests, toggleComments)
{
    QKeyEvent ctrlSlash = key(Qt::Key_Slash, Ctrl);
    QKeyEvent ctrlShiftC = key(Qt::Key_C, CtrlShift);
    QKeyEvent ctrlC = key(Qt::Key_C, Ctrl);
    QKeyEvent slash = key(Qt::Key_Slash);

    EXPECT_TRUE(KeyboardManager::isToggleCommentsShortcut(&ctrlSlash));
    EXPECT_TRUE(KeyboardManager::isToggleCommentsShortcut(&ctrlShiftC));
    EXPECT_FALSE(KeyboardManager::isToggleCommentsShortcut(&ctrlC));
    EXPECT_FALSE(KeyboardManager::isToggleCommentsShortcut(&slash));
}
