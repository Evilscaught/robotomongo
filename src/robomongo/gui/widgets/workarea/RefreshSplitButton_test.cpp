#include <gtest/gtest.h>

#include <QApplication>
#include <QImage>
#include <QPushButton>

#include "robomongo/gui/widgets/workarea/RefreshSplitButton.h"
#include "robomongo-unit-tests/QtTestApplication.h"

using namespace Robomongo;

namespace
{
    // Background colour of the toggle, sampled from its left padding (clear of text and corners)
    QColor toggleBackground(RefreshSplitButton &button)
    {
        QImage const image = button.toggleButton()->grab().toImage();
        return image.pixelColor(3, image.height() / 2);
    }

    bool isGreen(QColor const& color)
    {
        return color.green() > color.red() + 60 && color.green() > color.blue() + 20;
    }

    bool isRed(QColor const& color)
    {
        return color.red() > color.green() + 60 && color.red() > color.blue() + 60;
    }

    // Background of the refresh button, sampled from its left edge (clear of the icon)
    QColor refreshBackground(RefreshSplitButton &button)
    {
        QImage const image = button.refreshButton()->grab().toImage();
        return image.pixelColor(3, image.height() / 2);
    }

    bool isOrange(QColor const& color)
    {
        return color.red() > 220 && color.green() > 120 && color.green() < 225 &&
               color.red() - color.blue() > 80;
    }

    struct SignalCounts
    {
        int refreshRequested = 0;
        std::vector<bool> toggled;
    };

    void track(RefreshSplitButton &button, SignalCounts &counts)
    {
        QObject::connect(&button, &RefreshSplitButton::refreshRequested,
                         [&counts]() { ++counts.refreshRequested; });
        QObject::connect(&button, &RefreshSplitButton::autoRefreshToggled,
                         [&counts](bool enabled) { counts.toggled.push_back(enabled); });
    }
}

class RefreshSplitButtonTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
};

TEST_F(RefreshSplitButtonTests, showsEnabledStateWhenAutoRefreshIsOn)
{
    RefreshSplitButton button(true);

    EXPECT_TRUE(button.autoRefresh());
    EXPECT_TRUE(button.toggleButton()->isChecked());
    EXPECT_EQ(QString("Refresh Enabled"), button.toggleButton()->text());
    EXPECT_FALSE(button.refreshButton()->isEnabled());
}

TEST_F(RefreshSplitButtonTests, showsDisabledStateWhenAutoRefreshIsOff)
{
    RefreshSplitButton button(false);

    EXPECT_FALSE(button.autoRefresh());
    EXPECT_FALSE(button.toggleButton()->isChecked());
    EXPECT_EQ(QString("Refresh Disabled"), button.toggleButton()->text());
    EXPECT_TRUE(button.refreshButton()->isEnabled());
}

TEST_F(RefreshSplitButtonTests, clickingToggleDisablesAutoRefresh)
{
    RefreshSplitButton button(true);
    SignalCounts counts;
    track(button, counts);

    button.toggleButton()->click();

    EXPECT_FALSE(button.autoRefresh());
    EXPECT_EQ(QString("Refresh Disabled"), button.toggleButton()->text());
    EXPECT_TRUE(button.refreshButton()->isEnabled());
    ASSERT_EQ(1u, counts.toggled.size());
    EXPECT_FALSE(counts.toggled[0]);
}

TEST_F(RefreshSplitButtonTests, clickingToggleEnablesAutoRefresh)
{
    RefreshSplitButton button(false);
    SignalCounts counts;
    track(button, counts);

    button.toggleButton()->click();

    EXPECT_TRUE(button.autoRefresh());
    EXPECT_EQ(QString("Refresh Enabled"), button.toggleButton()->text());
    EXPECT_FALSE(button.refreshButton()->isEnabled());
    ASSERT_EQ(1u, counts.toggled.size());
    EXPECT_TRUE(counts.toggled[0]);
}

TEST_F(RefreshSplitButtonTests, refreshButtonRequestsRefreshWhenAutoRefreshIsOff)
{
    RefreshSplitButton button(false);
    SignalCounts counts;
    track(button, counts);

    button.refreshButton()->click();

    EXPECT_EQ(1, counts.refreshRequested);
    EXPECT_TRUE(counts.toggled.empty());
}

TEST_F(RefreshSplitButtonTests, refreshButtonDoesNothingWhileAutoRefreshIsOn)
{
    RefreshSplitButton button(true);
    SignalCounts counts;
    track(button, counts);

    button.refreshButton()->click();

    EXPECT_EQ(0, counts.refreshRequested);
}

TEST_F(RefreshSplitButtonTests, splitsWidthTwoThirdsToOneThird)
{
    RefreshSplitButton button(true);
    int const toggleWidth = button.toggleButton()->width();
    int const refreshWidth = button.refreshButton()->width();

    EXPECT_GT(refreshWidth, 0);
    EXPECT_EQ(toggleWidth, 2 * refreshWidth);
    EXPECT_EQ(button.toggleButton()->height(), button.refreshButton()->height());
}

TEST_F(RefreshSplitButtonTests, widthIsStableAndFitsBothLabels)
{
    RefreshSplitButton button(true);
    button.toggleButton()->ensurePolished();    // apply the stylesheet's bold font
    int const widthBefore = button.toggleButton()->width();
    auto const metrics = button.toggleButton()->fontMetrics();
    int const horizontalPadding = 2 * 6;

    button.toggleButton()->click();

    EXPECT_EQ(widthBefore, button.toggleButton()->width());
    EXPECT_GE(widthBefore, metrics.horizontalAdvance("Refresh Disabled") + horizontalPadding);
    EXPECT_GE(widthBefore, metrics.horizontalAdvance("Refresh Enabled") + horizontalPadding);
}

TEST_F(RefreshSplitButtonTests, fitsBothLabelsAtLargeFontSize)
{
    // A large font widens the gap between the two labels beyond the fixed padding,
    // so sizing for only one label would clip the other.
    QFont const originalFont = QApplication::font();
    QFont largeFont = originalFont;
    largeFont.setPointSize(36);
    QApplication::setFont(largeFont);

    RefreshSplitButton button(true);
    button.toggleButton()->ensurePolished();
    int const width = button.toggleButton()->width();
    auto const metrics = button.toggleButton()->fontMetrics();
    int const horizontalPadding = 2 * 6;

    QApplication::setFont(originalFont);

    EXPECT_GE(width, metrics.horizontalAdvance("Refresh Disabled") + horizontalPadding);
    EXPECT_GE(width, metrics.horizontalAdvance("Refresh Enabled") + horizontalPadding);
}

TEST_F(RefreshSplitButtonTests, rendersGreenWhenEnabledAndRedWhenDisabled)
{
    RefreshSplitButton enabled(true);
    RefreshSplitButton disabled(false);

    QColor const enabledColor = toggleBackground(enabled);
    QColor const disabledColor = toggleBackground(disabled);

    EXPECT_TRUE(isGreen(enabledColor)) << enabledColor.name().toStdString();
    EXPECT_TRUE(isRed(disabledColor)) << disabledColor.name().toStdString();
}

TEST_F(RefreshSplitButtonTests, colourFollowsToggle)
{
    RefreshSplitButton button(true);

    button.toggleButton()->click();
    QColor const afterDisable = toggleBackground(button);
    button.toggleButton()->click();
    QColor const afterEnable = toggleBackground(button);

    EXPECT_TRUE(isRed(afterDisable)) << afterDisable.name().toStdString();
    EXPECT_TRUE(isGreen(afterEnable)) << afterEnable.name().toStdString();
}

TEST_F(RefreshSplitButtonTests, refreshButtonIsNotOutdatedByDefault)
{
    RefreshSplitButton button(false);
    QColor const background = refreshBackground(button);

    EXPECT_FALSE(button.isOutdated());
    EXPECT_FALSE(button.showsOutdated());
    EXPECT_EQ(QString("Refresh results"), button.refreshButton()->toolTip());
    EXPECT_FALSE(isOrange(background)) << background.name().toStdString();
}

TEST_F(RefreshSplitButtonTests, outdatedResultsGlowOrangeWithExplanation)
{
    RefreshSplitButton button(false);
    refreshBackground(button);     // already on screen before documents change

    button.setOutdated(true);
    QColor const background = refreshBackground(button);

    EXPECT_TRUE(button.showsOutdated());
    EXPECT_TRUE(button.refreshButton()->isEnabled());
    EXPECT_TRUE(button.refreshButton()->toolTip().contains("out of date"));
    EXPECT_TRUE(isOrange(background)) << background.name().toStdString();
}

TEST_F(RefreshSplitButtonTests, clearingOutdatedRemovesGlow)
{
    RefreshSplitButton button(false);
    button.setOutdated(true);
    QColor const whileOutdated = refreshBackground(button);

    button.setOutdated(false);
    QColor const background = refreshBackground(button);

    EXPECT_FALSE(button.showsOutdated());
    EXPECT_EQ(QString("Refresh results"), button.refreshButton()->toolTip());
    EXPECT_TRUE(isOrange(whileOutdated)) << whileOutdated.name().toStdString();
    EXPECT_FALSE(isOrange(background)) << background.name().toStdString();
}

TEST_F(RefreshSplitButtonTests, outdatedIsNotShownWhileAutoRefreshIsOn)
{
    RefreshSplitButton button(true);

    button.setOutdated(true);
    QColor const background = refreshBackground(button);

    EXPECT_FALSE(button.showsOutdated());
    EXPECT_FALSE(button.refreshButton()->toolTip().contains("out of date"));
    EXPECT_FALSE(isOrange(background)) << background.name().toStdString();
}

TEST_F(RefreshSplitButtonTests, enablingAutoRefreshRefreshesOutdatedResults)
{
    RefreshSplitButton button(false);
    button.setOutdated(true);
    SignalCounts counts;
    track(button, counts);

    button.toggleButton()->click();

    EXPECT_EQ(1, counts.refreshRequested);
    EXPECT_FALSE(button.showsOutdated());
    ASSERT_EQ(1u, counts.toggled.size());
    EXPECT_TRUE(counts.toggled[0]);
}

TEST_F(RefreshSplitButtonTests, enablingAutoRefreshDoesNotRefreshCurrentResults)
{
    RefreshSplitButton button(false);
    SignalCounts counts;
    track(button, counts);

    button.toggleButton()->click();

    EXPECT_EQ(0, counts.refreshRequested);
}

TEST_F(RefreshSplitButtonTests, disablingAutoRefreshShowsPendingOutdatedState)
{
    RefreshSplitButton button(true);
    button.setOutdated(true);
    refreshBackground(button);     // drawn while auto-refresh hides the indicator
    SignalCounts counts;
    track(button, counts);

    button.toggleButton()->click();
    QColor const background = refreshBackground(button);

    EXPECT_TRUE(button.showsOutdated());
    EXPECT_EQ(0, counts.refreshRequested);
    EXPECT_TRUE(isOrange(background)) << background.name().toStdString();
}
