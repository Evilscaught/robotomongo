// Runs in robo_isolated_tests (HOME is a scratch directory; see IsolatedMain.cpp)

#include <gtest/gtest.h>

#include <QLineEdit>
#include <QPushButton>
#include <QValidator>

#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/gui/widgets/workarea/PagingWidget.h"

using namespace Robomongo;

namespace
{
    struct Emitted
    {
        std::vector<std::pair<int, int>> left, right, refreshed;
    };

    void track(PagingWidget &widget, Emitted &emitted)
    {
        QObject::connect(&widget, &PagingWidget::leftClicked,
                         [&emitted](int skip, int limit) { emitted.left.push_back({ skip, limit }); });
        QObject::connect(&widget, &PagingWidget::rightClicked,
                         [&emitted](int skip, int limit) { emitted.right.push_back({ skip, limit }); });
        QObject::connect(&widget, &PagingWidget::refreshed,
                         [&emitted](int skip, int limit) { emitted.refreshed.push_back({ skip, limit }); });
    }

    QList<QLineEdit *> edits(PagingWidget &widget) { return widget.findChildren<QLineEdit *>(); }

    // Layout order: [<] [skip] [batch size] [>]
    QList<QPushButton *> buttons(PagingWidget &widget) { return widget.findChildren<QPushButton *>(); }
}

class PagingWidgetTests : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ensureQtTestApplication();
        AppRegistry::instance().settingsManager()->setBatchSize(50);
    }
};

TEST_F(PagingWidgetTests, showsSkipAndBatchSize)
{
    PagingWidget widget;

    widget.setSkip(20);
    widget.setBatchSize(10);

    EXPECT_EQ(10, widget.batchSize());
    ASSERT_EQ(2, edits(widget).size());
    EXPECT_EQ(QString("20"), edits(widget)[0]->text());
    EXPECT_EQ(QString("10"), edits(widget)[1]->text());
}

TEST_F(PagingWidgetTests, nonPositiveBatchSizeUsesConfiguredDefault)
{
    PagingWidget widget;

    widget.setBatchSize(0);
    EXPECT_EQ(50, widget.batchSize());

    widget.setBatchSize(-5);
    EXPECT_EQ(50, widget.batchSize());
}

TEST_F(PagingWidgetTests, arrowsReportCurrentPage)
{
    PagingWidget widget;
    widget.setSkip(40);
    widget.setBatchSize(20);
    Emitted emitted;
    track(widget, emitted);
    ASSERT_EQ(2, buttons(widget).size());

    buttons(widget)[0]->click();
    buttons(widget)[1]->click();

    EXPECT_EQ((std::vector<std::pair<int, int>>{ { 40, 20 } }), emitted.left);
    EXPECT_EQ((std::vector<std::pair<int, int>>{ { 40, 20 } }), emitted.right);
    EXPECT_TRUE(emitted.refreshed.empty());
}

TEST_F(PagingWidgetTests, enterInEitherBoxRefreshesWithTypedValues)
{
    PagingWidget widget;
    widget.setSkip(0);
    widget.setBatchSize(50);
    Emitted emitted;
    track(widget, emitted);

    edits(widget)[0]->setText("100");
    emit edits(widget)[0]->returnPressed();
    edits(widget)[1]->setText("25");
    emit edits(widget)[1]->returnPressed();

    EXPECT_EQ((std::vector<std::pair<int, int>>{ { 100, 50 }, { 100, 25 } }), emitted.refreshed);
}

TEST_F(PagingWidgetTests, boxesOnlyAcceptDigits)
{
    PagingWidget widget;

    for (QLineEdit *edit : edits(widget)) {
        QString text = "12a";
        int position = 0;
        EXPECT_EQ(QValidator::Invalid, edit->validator()->validate(text, position));
        text = "-5";
        EXPECT_EQ(QValidator::Invalid, edit->validator()->validate(text, position));
        text = "123";
        EXPECT_EQ(QValidator::Acceptable, edit->validator()->validate(text, position));
    }
}

TEST_F(PagingWidgetTests, settingValuesShowsTheWidget)
{
    PagingWidget widget;
    widget.hide();

    widget.setSkip(0);

    EXPECT_FALSE(widget.isHidden());
}
