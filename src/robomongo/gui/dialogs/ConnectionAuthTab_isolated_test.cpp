// Runs in robo_isolated_tests (HOME is a scratch directory; see IsolatedMain.cpp)

#include <gtest/gtest.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/gui/dialogs/ConnectionAuthTab.h"
#include "robomongo/gui/utils/GuiConstants.h"

using namespace Robomongo;

namespace
{
    // Fields by their position in the tab's grid layout
    struct AuthFields
    {
        explicit AuthFields(ConnectionAuthTab &tab)
        {
            auto grid = static_cast<QGridLayout *>(tab.layout());
            auto at = [grid](int row, int column) { return grid->itemAtPosition(row, column)->widget(); };
            useAuth = qobject_cast<QCheckBox *>(at(0, 0));
            database = qobject_cast<QLineEdit *>(at(1, 1));
            userName = qobject_cast<QLineEdit *>(at(4, 1));
            password = qobject_cast<QLineEdit *>(at(5, 1));
            showPassword = qobject_cast<QPushButton *>(at(5, 2));
            mechanism = qobject_cast<QComboBox *>(at(6, 1));
            useVisibleDbs = qobject_cast<QCheckBox *>(at(9, 0));
            visibleDbs = qobject_cast<QLineEdit *>(at(10, 1));
        }

        QCheckBox *useAuth, *useVisibleDbs;
        QLineEdit *database, *userName, *password, *visibleDbs;
        QComboBox *mechanism;
        QPushButton *showPassword;
    };

    CredentialSettings *credential(bool enabled = true)
    {
        auto credential = new CredentialSettings();
        credential->setUserName("ada");
        credential->setUserPassword("s3cret");
        credential->setDatabaseName("shop");
        credential->setMechanism("SCRAM-SHA-256");
        credential->setEnabled(enabled);
        credential->setUseManuallyVisibleDbs(true);
        credential->setManuallyVisibleDbs("shop,logs");
        return credential;
    }
}

class ConnectionAuthTabTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
    ConnectionSettings settings { false };
};

TEST_F(ConnectionAuthTabTests, findsAllFields)
{
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);

    for (QWidget *widget : std::initializer_list<QWidget *>{ fields.useAuth, fields.database, fields.userName,
                                                             fields.password, fields.showPassword, fields.mechanism,
                                                             fields.useVisibleDbs, fields.visibleDbs })
        EXPECT_NE(nullptr, widget);
}

TEST_F(ConnectionAuthTabTests, newConnectionHasAuthOffAndAdminDatabase)
{
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);

    EXPECT_FALSE(fields.useAuth->isChecked());
    EXPECT_EQ(QString("admin"), fields.database->text());
    EXPECT_EQ(QString(""), fields.userName->text());
    EXPECT_EQ(QString("SCRAM-SHA-1"), fields.mechanism->currentText());
}

TEST_F(ConnectionAuthTabTests, offersSupportedMechanisms)
{
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);

    ASSERT_EQ(3, fields.mechanism->count());
    EXPECT_EQ(QString("SCRAM-SHA-1"), fields.mechanism->itemText(0));
    EXPECT_EQ(QString("SCRAM-SHA-256"), fields.mechanism->itemText(1));
    EXPECT_EQ(QString("MONGODB-CR"), fields.mechanism->itemText(2));
}

TEST_F(ConnectionAuthTabTests, showsExistingCredential)
{
    settings.addCredential(credential());
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);

    EXPECT_TRUE(fields.useAuth->isChecked());
    EXPECT_EQ(QString("shop"), fields.database->text());
    EXPECT_EQ(QString("ada"), fields.userName->text());
    EXPECT_EQ(QString("s3cret"), fields.password->text());
    EXPECT_EQ(QString("SCRAM-SHA-256"), fields.mechanism->currentText());
    EXPECT_TRUE(fields.useVisibleDbs->isChecked());
    EXPECT_EQ(QString("shop,logs"), fields.visibleDbs->text());
}

TEST_F(ConnectionAuthTabTests, passwordIsMaskedUntilRevealed)
{
    settings.addCredential(credential());
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);

    EXPECT_EQ(QLineEdit::Password, fields.password->echoMode());
    fields.showPassword->click();
    EXPECT_EQ(QLineEdit::Normal, fields.password->echoMode());
    fields.showPassword->click();
    EXPECT_EQ(QLineEdit::Password, fields.password->echoMode());
}

TEST_F(ConnectionAuthTabTests, acceptSavesTypedCredential)
{
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);
    fields.useAuth->setChecked(true);
    fields.database->setText("reports");
    fields.userName->setText("grace");
    fields.password->setText("pä$$ wörd");
    fields.mechanism->setCurrentIndex(1);

    tab.accept();

    ASSERT_EQ(1, settings.credentialCount());
    CredentialSettings const *saved = settings.primaryCredential();
    EXPECT_TRUE(saved->enabled());
    EXPECT_EQ("reports", saved->databaseName());
    EXPECT_EQ("grace", saved->userName());
    EXPECT_EQ("pä$$ wörd", saved->userPassword());
    EXPECT_EQ("SCRAM-SHA-256", saved->mechanism());
}

TEST_F(ConnectionAuthTabTests, uncheckedAuthKeepsCredentialButDisablesIt)
{
    settings.addCredential(credential());
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);
    fields.useAuth->setChecked(false);

    tab.accept();

    ASSERT_EQ(1, settings.credentialCount());
    EXPECT_FALSE(settings.primaryCredential()->enabled());
    EXPECT_EQ("ada", settings.primaryCredential()->userName());
    EXPECT_FALSE(settings.hasEnabledPrimaryCredential());
}

TEST_F(ConnectionAuthTabTests, emptyFieldsRemoveCredential)
{
    settings.addCredential(credential());
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);
    fields.userName->clear();
    fields.password->clear();
    fields.database->clear();

    tab.accept();

    EXPECT_EQ(0, settings.credentialCount());
}

TEST_F(ConnectionAuthTabTests, visibleDatabasesAreSavedWithoutSpaces)
{
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);
    fields.userName->setText("ada");
    fields.useVisibleDbs->setChecked(true);
    fields.visibleDbs->setText(" shop ,  logs, analytics ");

    tab.accept();

    EXPECT_TRUE(settings.primaryCredential()->useManuallyVisibleDbs());
    EXPECT_EQ("shop,logs,analytics", settings.primaryCredential()->manuallyVisibleDbs());
}

TEST_F(ConnectionAuthTabTests, visibleDatabasesNeedAList)
{
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);
    fields.userName->setText("ada");
    fields.useVisibleDbs->setChecked(true);
    fields.visibleDbs->clear();

    tab.accept();

    EXPECT_FALSE(settings.primaryCredential()->useManuallyVisibleDbs());
}

TEST_F(ConnectionAuthTabTests, setAuthTabFillsFields)
{
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);

    tab.setAuthTab("admin", "root", "pw", MONGODB_CR);

    EXPECT_TRUE(fields.useAuth->isChecked());
    EXPECT_EQ(QString("root"), fields.userName->text());
    EXPECT_EQ(QString("pw"), fields.password->text());
    EXPECT_EQ(QString("MONGODB-CR"), fields.mechanism->currentText());
}

TEST_F(ConnectionAuthTabTests, clearTabEmptiesFields)
{
    settings.addCredential(credential());
    ConnectionAuthTab tab(&settings);
    AuthFields const fields(tab);

    tab.clearTab();

    EXPECT_FALSE(fields.useAuth->isChecked());
    EXPECT_TRUE(fields.userName->text().isEmpty());
    EXPECT_TRUE(fields.password->text().isEmpty());
    EXPECT_TRUE(fields.database->text().isEmpty());
    EXPECT_FALSE(fields.useVisibleDbs->isChecked());
    EXPECT_EQ(0, fields.mechanism->currentIndex());
}

TEST(AuthMechanismTests, parsesMechanismNames)
{
    EXPECT_EQ(SCRAM_SHA_1, authMechanismFromStr("SCRAM-SHA-1"));
    EXPECT_EQ(SCRAM_SHA_256, authMechanismFromStr("SCRAM-SHA-256"));
    EXPECT_EQ(MONGODB_CR, authMechanismFromStr("MONGODB-CR"));
}

TEST(AuthMechanismTests, unknownNamesFallBackToScramSha1)
{
    EXPECT_EQ(SCRAM_SHA_1, authMechanismFromStr(""));
    EXPECT_EQ(SCRAM_SHA_1, authMechanismFromStr("scram-sha-256"));
    EXPECT_EQ(SCRAM_SHA_1, authMechanismFromStr("PLAIN"));
}
