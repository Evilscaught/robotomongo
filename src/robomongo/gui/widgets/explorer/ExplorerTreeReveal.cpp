#include "robomongo/gui/widgets/explorer/ExplorerTreeReveal.h"

#include <QSignalBlocker>
#include <QTreeWidget>

namespace Robomongo
{
    void revealExplorerItem(QTreeWidget *tree, QTreeWidgetItem *item)
    {
        if (!tree || !item || item->treeWidget() != tree)
            return;

        QSignalBlocker const blocker(tree);

        for (QTreeWidgetItem *parent = item->parent(); parent; parent = parent->parent())
            parent->setExpanded(true);

        tree->setCurrentItem(item);
        tree->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
}
