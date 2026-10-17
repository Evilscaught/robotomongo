#include "robomongo/gui/widgets/workarea/RefreshSplitButton.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>

#include <algorithm>

#include "robomongo/core/utils/QtUtils.h"

namespace
{
    const char *const EnabledText = "Refresh Enabled";
    const char *const DisabledText = "Refresh Disabled";
    const int ButtonHeight = 22;
}

namespace Robomongo
{
    RefreshSplitButton::RefreshSplitButton(bool autoRefresh, QWidget *parent) :
        QFrame(parent)
    {
        setStyleSheet(
            "QPushButton#autoRefreshToggle {"
            "  color: white; font-weight: 600; border: none; padding: 0 6px;"
            "  border-top-left-radius: 4px; border-bottom-left-radius: 4px; }"
            "QPushButton#autoRefreshToggle:checked { background: #2e9e5b; }"
            "QPushButton#autoRefreshToggle:checked:hover { background: #278a4e; }"
            "QPushButton#autoRefreshToggle:!checked { background: #c9473d; }"
            "QPushButton#autoRefreshToggle:!checked:hover { background: #b03d34; }"
            "QPushButton#manualRefresh {"
            "  border: 1px solid #b0b0b0; border-left: none; background: palette(button);"
            "  border-top-right-radius: 4px; border-bottom-right-radius: 4px; }"
            "QPushButton#manualRefresh:hover { background: palette(midlight); }"
            "QPushButton#manualRefresh:pressed { background: palette(mid); }"
            "QPushButton#manualRefresh:disabled { background: palette(window); }");

        _toggleButton = new QPushButton(this);
        _toggleButton->setObjectName("autoRefreshToggle");
        _toggleButton->setCheckable(true);
        _toggleButton->setCursor(Qt::PointingHandCursor);

        _refreshButton = new QPushButton(this);
        _refreshButton->setObjectName("manualRefresh");
        _refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        _refreshButton->setIconSize(QSize(14, 14));
        _refreshButton->setToolTip("Refresh results");

        // Rule of thirds: toggle takes 2/3 of the width, refresh button 1/3.
        // Sized for the longer label so the width does not change when toggled.
        // Polish first so the measurement uses the stylesheet's bold font.
        _toggleButton->ensurePolished();
        auto const metrics = _toggleButton->fontMetrics();
        int const textWidth = std::max(metrics.horizontalAdvance(EnabledText),
                                       metrics.horizontalAdvance(DisabledText));
        int const refreshWidth = (textWidth + 20) / 2;
        _toggleButton->setFixedSize(refreshWidth * 2, ButtonHeight);
        _refreshButton->setFixedSize(refreshWidth, ButtonHeight);

        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(_toggleButton);
        layout->addWidget(_refreshButton);

        _toggleButton->setChecked(autoRefresh);
        updateState(autoRefresh);

        VERIFY(connect(_toggleButton, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool))));
        VERIFY(connect(_refreshButton, SIGNAL(clicked()), this, SIGNAL(refreshRequested())));
    }

    bool RefreshSplitButton::autoRefresh() const
    {
        return _toggleButton->isChecked();
    }

    void RefreshSplitButton::onToggled(bool enabled)
    {
        updateState(enabled);
        emit autoRefreshToggled(enabled);
    }

    void RefreshSplitButton::updateState(bool autoRefresh)
    {
        _toggleButton->setText(autoRefresh ? EnabledText : DisabledText);
        _toggleButton->setToolTip(autoRefresh
            ? "Results re-run automatically after inserting, editing or deleting documents.\n"
              "Click to disable."
            : "Results are not re-run after inserting, editing or deleting documents.\n"
              "Click to enable, or use the refresh button.");
        _refreshButton->setEnabled(!autoRefresh);
    }
}
