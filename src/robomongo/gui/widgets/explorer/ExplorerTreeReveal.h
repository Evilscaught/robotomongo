#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Robomongo
{
    /**
     * @brief Selects `item` and scrolls it into view, expanding its ancestors.
     *        Expansion happens with the tree's signals blocked: explorer folders reload
     *        (and recreate their children) when itemExpanded fires, which would delete `item`.
     *        Keyboard focus is left where it is.
     */
    void revealExplorerItem(QTreeWidget *tree, QTreeWidgetItem *item);
}
