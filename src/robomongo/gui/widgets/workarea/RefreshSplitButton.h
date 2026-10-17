#pragma once

#include <QFrame>
QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace Robomongo
{
    /**
     * @brief Split button shown above query results.
     *        Left 2/3 toggles auto-refresh after writes ("Refresh Enabled" in green,
     *        "Refresh Disabled" in red). Right 1/3 refreshes results manually and is
     *        only enabled while auto-refresh is disabled.
     */
    class RefreshSplitButton : public QFrame
    {
        Q_OBJECT

    public:
        explicit RefreshSplitButton(bool autoRefresh, QWidget *parent = nullptr);

        bool autoRefresh() const;
        QPushButton *toggleButton() const { return _toggleButton; }
        QPushButton *refreshButton() const { return _refreshButton; }

    Q_SIGNALS:
        void autoRefreshToggled(bool enabled);
        void refreshRequested();

    private Q_SLOTS:
        void onToggled(bool enabled);

    private:
        void updateState(bool autoRefresh);

        QPushButton *_toggleButton;
        QPushButton *_refreshButton;
    };
}
