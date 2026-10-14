#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

#include "robomongo/core/domain/Notifier.h"

namespace Robomongo
{
    class FindFrame;

    /**
     * @brief Shown in place of the document views while a find on a collection has no
     *        documents. Displays the plain output page, with "Insert Document..." added to
     *        its right-click menu (ahead of the usual Copy and Select All).
     *        Acts as the notifier observer for that insert action; there is never a selection.
     */
    class EmptyResultsView : public QWidget, public INotifierObserver
    {
        Q_OBJECT

    public:
        EmptyResultsView(FindFrame *output, QWidget *parent = nullptr);

        FindFrame *output() const { return _output; }

        // Action placed first in the context menu; nullptr when inserting is not possible
        void setInsertDocumentAction(QAction *action) { _insertDocumentAction = action; }
        void populateContextMenu(QMenu *menu) const;

        QModelIndex selectedIndex() const override { return QModelIndex(); }
        QModelIndexList selectedIndexes() const override { return QModelIndexList(); }

    private Q_SLOTS:
        void showContextMenu(const QPoint &point);

    private:
        FindFrame *const _output;
        QAction *_insertDocumentAction;
    };
}
