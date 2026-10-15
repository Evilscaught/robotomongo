#include <gtest/gtest.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/gui/dialogs/ResultExportDialog.h"

using namespace Robomongo;

namespace
{
    // Children in construction order: visible, limit, all
    QList<QRadioButton *> scopes(ResultExportDialog &dialog) { return dialog.findChildren<QRadioButton *>(); }
    QComboBox *format(ResultExportDialog &dialog) { return dialog.findChild<QComboBox *>(); }
    QSpinBox *limit(ResultExportDialog &dialog) { return dialog.findChild<QSpinBox *>(); }
    QLineEdit *path(ResultExportDialog &dialog) { return dialog.findChild<QLineEdit *>(); }
}

class ResultExportDialogTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
};

TEST_F(ResultExportDialogTests, defaultsToVisibleResultsAsJson)
{
    ResultExportDialog dialog(true, 25, 50);

    ResultExportSelection const selection = dialog.selection();

    EXPECT_EQ(QueryResultExportFormat::Json, selection.format);
    EXPECT_EQ(ResultExportScope::VisibleResults, selection.scope);
    EXPECT_EQ(QString("Visible results (25)"), scopes(dialog)[0]->text());
    EXPECT_TRUE(scopes(dialog)[0]->isChecked());
}

TEST_F(ResultExportDialogTests, offersAllThreeFormats)
{
    ResultExportDialog dialog(true, 1, 50);

    ASSERT_EQ(3, format(dialog)->count());
    EXPECT_EQ(QString("JSON"), format(dialog)->itemText(0));
    EXPECT_EQ(QString("JSON Lines"), format(dialog)->itemText(1));
    EXPECT_EQ(QString("CSV"), format(dialog)->itemText(2));
}

TEST_F(ResultExportDialogTests, queryScopesNeedAFindQuery)
{
    ResultExportDialog dialog(false, 10, 50);

    EXPECT_TRUE(scopes(dialog)[0]->isEnabled());
    EXPECT_FALSE(scopes(dialog)[1]->isEnabled());
    EXPECT_FALSE(scopes(dialog)[2]->isEnabled());
    EXPECT_FALSE(limit(dialog)->isEnabled());
}

TEST_F(ResultExportDialogTests, limitScopeUsesSpinBoxValue)
{
    ResultExportDialog dialog(true, 10, 50);
    scopes(dialog)[1]->setChecked(true);

    EXPECT_TRUE(limit(dialog)->isEnabled());
    EXPECT_EQ(50, limit(dialog)->value());
    limit(dialog)->setValue(500);

    ResultExportSelection const selection = dialog.selection();
    EXPECT_EQ(ResultExportScope::LimitedQueryResults, selection.scope);
    EXPECT_EQ(500, selection.limit);
}

TEST_F(ResultExportDialogTests, defaultLimitFallsBackToVisibleCountThenFifty)
{
    ResultExportDialog fromVisible(true, 30, 0);
    ResultExportDialog fromNothing(true, 0, 0);

    EXPECT_EQ(30, limit(fromVisible)->value());
    EXPECT_EQ(50, limit(fromNothing)->value());
}

TEST_F(ResultExportDialogTests, allScope)
{
    ResultExportDialog dialog(true, 10, 50);
    scopes(dialog)[2]->setChecked(true);

    EXPECT_EQ(ResultExportScope::AllQueryResults, dialog.selection().scope);
    EXPECT_FALSE(limit(dialog)->isEnabled());
}

TEST_F(ResultExportDialogTests, selectedFormatIsReported)
{
    ResultExportDialog dialog(true, 10, 50);

    format(dialog)->setCurrentIndex(1);
    EXPECT_EQ(QueryResultExportFormat::JsonLines, dialog.selection().format);
    format(dialog)->setCurrentIndex(2);
    EXPECT_EQ(QueryResultExportFormat::Csv, dialog.selection().format);
}

TEST_F(ResultExportDialogTests, placeholderShowsExpectedExtension)
{
    ResultExportDialog dialog(true, 10, 50);

    EXPECT_EQ(QString("Choose a .json file"), path(dialog)->placeholderText());
    format(dialog)->setCurrentIndex(2);
    EXPECT_EQ(QString("Choose a .csv file"), path(dialog)->placeholderText());
}

TEST_F(ResultExportDialogTests, acceptAddsMissingExtensionForFormat)
{
    for (int index : { 0, 1, 2 }) {
        ResultExportDialog dialog(true, 10, 50);
        format(dialog)->setCurrentIndex(index);
        path(dialog)->setText("  /tmp/export  ");

        dialog.accept();

        QString const expected[] = { "/tmp/export.json", "/tmp/export.jsonl", "/tmp/export.csv" };
        EXPECT_EQ(expected[index], dialog.selection().filePath);
        EXPECT_EQ(QDialog::Accepted, dialog.result());
    }
}

TEST_F(ResultExportDialogTests, acceptKeepsExistingExtension)
{
    ResultExportDialog dialog(true, 10, 50);
    format(dialog)->setCurrentIndex(2);
    path(dialog)->setText("/tmp/report.txt");

    dialog.accept();

    EXPECT_EQ(QString("/tmp/report.txt"), dialog.selection().filePath);
}

TEST_F(ResultExportDialogTests, saveButtonIsLabelledExport)
{
    ResultExportDialog dialog(true, 10, 50);

    QDialogButtonBox *buttons = dialog.findChild<QDialogButtonBox *>();
    ASSERT_NE(nullptr, buttons);
    EXPECT_EQ(QString("Export"), buttons->button(QDialogButtonBox::Save)->text());
}
