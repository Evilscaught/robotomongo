#include "robomongo/gui/widgets/workarea/EmptyResultsView.h"

#include <QAction>
#include <QMenu>
#include <QVBoxLayout>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"

namespace Robomongo
{
    EmptyResultsView::EmptyResultsView(FindFrame *output, QWidget *parent) :
        QWidget(parent),
        _output(output),
        _insertDocumentAction(nullptr)
    {
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(_output);

        auto editor = _output->sciScintilla();
        editor->setContextMenuPolicy(Qt::CustomContextMenu);
        VERIFY(connect(editor, SIGNAL(customContextMenuRequested(const QPoint &)),
                       this, SLOT(showContextMenu(const QPoint &))));
    }

    void EmptyResultsView::populateContextMenu(QMenu *menu) const
    {
        auto editor = _output->sciScintilla();

        if (_insertDocumentAction) {
            menu->addAction(_insertDocumentAction);
            menu->addSeparator();
        }

        QAction *copy = menu->addAction("Copy");
        copy->setEnabled(editor->hasSelectedText());
        VERIFY(connect(copy, SIGNAL(triggered()), editor, SLOT(copy())));

        QAction *selectAll = menu->addAction("Select All");
        VERIFY(connect(selectAll, SIGNAL(triggered()), editor, SLOT(selectAll())));
    }

    void EmptyResultsView::showContextMenu(const QPoint &point)
    {
        QMenu menu(this);
        populateContextMenu(&menu);
        menu.exec(_output->sciScintilla()->mapToGlobal(point));
    }
}
