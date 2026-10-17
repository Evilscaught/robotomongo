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
     *        only enabled while auto-refresh is disabled. When documents change while
     *        auto-refresh is disabled, the refresh button glows orange until results reload.
     */
    class RefreshSplitButton : public QFrame
    {
        Q_OBJECT

    public:
        explicit RefreshSplitButton(bool autoRefresh, QWidget *parent = nullptr);

        bool autoRefresh() const;

        // Mark results as no longer matching the database (cleared when results reload)
        void setOutdated(bool outdated);
        bool isOutdated() const { return _outdated; }
        // Outdated indicator is only shown while auto-refresh is disabled
        bool showsOutdated() const;

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
        bool _outdated;
    };
}
