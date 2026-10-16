// Runs in robo_isolated_tests (HOME is a scratch directory; see IsolatedMain.cpp)

#include <gtest/gtest.h>

#include <memory>

#include <QDir>
#include <QFile>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"

using namespace Robomongo;

namespace
{
    QString readConfigFile()
    {
        QFile file(ConfigFilePath);
        if (!file.open(QIODevice::ReadOnly))
            return QString();
        return QString::fromUtf8(file.readAll());
    }

    void writeConfigFile(const QByteArray &content)
    {
        QDir().mkpath(ConfigDir);
        QFile file(ConfigFilePath);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(content);
    }

    ConnectionSettings *connection(const std::string &name, const std::string &password)
    {
        auto settings = new ConnectionSettings(false);
        settings->setConnectionName(name);
        settings->setServerHost(name + ".example.com");
        settings->setServerPort(27018);
        auto credential = new CredentialSettings();
        credential->setUserName("admin");
        credential->setUserPassword(password);
        credential->setEnabled(true);
        settings->addCredential(credential);
        settings->sshSettings()->setEnabled(true);
        settings->sshSettings()->setUserPassword(password + "-ssh");
        settings->setUuid(QString::fromStdString("{" + name + "}"));
        return settings;
    }
}

namespace
{
    // SettingsManager keeps connections in a static list and its destructor deletes them without
    // clearing the list, so a later instance would see dangling pointers. The app only ever has
    // one instance; tests remove the connections before destroying each manager.
    class Manager
    {
    public:
        Manager() : _settings(new SettingsManager()) {}
        ~Manager()
        {
            for (ConnectionSettings *connection : _settings->connections())
                _settings->removeConnection(connection);
            delete _settings;
        }
        SettingsManager *operator->() const { return _settings; }
        SettingsManager *get() const { return _settings; }

    private:
        SettingsManager *const _settings;
    };
}

class SettingsManagerTests : public ::testing::Test
{
protected:
    // Every test starts without a config file. SettingsManager keeps connections in a static list,
    // so only one instance may be alive at a time.
    void SetUp() override
    {
        ASSERT_TRUE(QDir::homePath().contains("robo-test-home"));
        QFile::remove(ConfigFilePath);
    }
};

TEST_F(SettingsManagerTests, configLivesInTheScratchHome)
{
    EXPECT_TRUE(ConfigFilePath.startsWith(QDir::homePath()));
    EXPECT_TRUE(ConfigFilePath.endsWith("/robo3t.json"));
}

TEST_F(SettingsManagerTests, createsConfigWithDefaults)
{
    Manager const settings;

    EXPECT_TRUE(QFile::exists(ConfigFilePath));
    EXPECT_EQ(DefaultEncoding, settings->uuidEncoding());
    EXPECT_EQ(Utc, settings->timeZone());
    EXPECT_EQ(Tree, settings->viewMode());
    EXPECT_EQ(AutocompleteAll, settings->autocompletionMode());
    EXPECT_FALSE(settings->lineNumbers());
    EXPECT_FALSE(settings->loadMongoRcJs());
    EXPECT_EQ(50, settings->batchSize());
    EXPECT_EQ(10, settings->mongoTimeoutSec());
    EXPECT_EQ(15, settings->shellTimeoutSec());
    EXPECT_TRUE(settings->checkForUpdates());
    EXPECT_TRUE(settings->autoRefreshResults());
    EXPECT_TRUE(settings->explorerAutoReveal());
    EXPECT_FALSE(settings->explorerShowCollectionSizes());
    EXPECT_FALSE(settings->disableHttpsFeatures());
    EXPECT_TRUE(settings->connections().empty());
}

TEST_F(SettingsManagerTests, createsAnonymousIdOnce)
{
    QString firstId;
    {
        Manager const settings;
        firstId = settings->anonymousID();
        ASSERT_TRUE(settings->save());
    }
    Manager const reloaded;

    EXPECT_FALSE(firstId.isEmpty());
    EXPECT_EQ(firstId, reloaded->anonymousID());
}

TEST_F(SettingsManagerTests, preferencesRoundTripThroughConfigFile)
{
    {
        Manager settings;
        settings->setUuidEncoding(JavaLegacy);
        settings->setTimeZone(LocalTime);
        settings->setViewMode(Table);
        settings->setAutocompletionMode(AutocompleteNoCollectionNames);
        settings->setAutoExpand(false);
        settings->setAutoExec(false);
        settings->setLineNumbers(true);
        settings->setLoadMongoRcJs(true);
        settings->setDisableConnectionShortcuts(true);
        settings->setBatchSize(200);
        settings->setShellTimeoutSec(90);
        settings->setCheckForUpdates(false);
        settings->setAutoRefreshResults(false);
        settings->setExplorerAutoReveal(false);
        settings->setExplorerShowCollectionSizes(true);
        settings->setTextFontFamily("Menlo");
        settings->setTextFontPointSize(14);
        settings->setToolbarSettings("logs", true);
        ASSERT_TRUE(settings->save());
    }

    Manager const loaded;

    EXPECT_EQ(JavaLegacy, loaded->uuidEncoding());
    EXPECT_EQ(LocalTime, loaded->timeZone());
    EXPECT_EQ(Table, loaded->viewMode());
    EXPECT_EQ(AutocompleteNoCollectionNames, loaded->autocompletionMode());
    EXPECT_FALSE(loaded->autoExpand());
    EXPECT_FALSE(loaded->autoExec());
    EXPECT_TRUE(loaded->lineNumbers());
    EXPECT_TRUE(loaded->loadMongoRcJs());
    EXPECT_TRUE(loaded->disableConnectionShortcuts());
    EXPECT_EQ(200, loaded->batchSize());
    EXPECT_EQ(90, loaded->shellTimeoutSec());
    EXPECT_FALSE(loaded->checkForUpdates());
    EXPECT_FALSE(loaded->autoRefreshResults());
    EXPECT_FALSE(loaded->explorerAutoReveal());
    EXPECT_TRUE(loaded->explorerShowCollectionSizes());
    EXPECT_EQ(QString("Menlo"), loaded->textFontFamily());
    EXPECT_EQ(14, loaded->textFontPointSize());
    EXPECT_TRUE(loaded->toolbars().value("logs").toBool());
}

TEST_F(SettingsManagerTests, connectionsRoundTripThroughConfigFile)
{
    {
        Manager settings;
        SettingsManager::addConnection(connection("production", "prod-secret"));
        SettingsManager::addConnection(connection("staging", "stage-secret"));
        ASSERT_TRUE(settings->save());
    }

    Manager const loaded;

    ASSERT_EQ(2u, loaded->connections().size());
    ConnectionSettings const *production = loaded->connections()[0];
    EXPECT_EQ("production", production->connectionName());
    EXPECT_EQ("production.example.com", production->serverHost());
    EXPECT_EQ(27018, production->serverPort());
    EXPECT_EQ("prod-secret", production->primaryCredential()->userPassword());
    EXPECT_EQ("prod-secret-ssh", production->sshSettings()->userPassword());
    EXPECT_EQ("staging", loaded->connections()[1]->connectionName());
}

TEST_F(SettingsManagerTests, configFileHasNoPlainTextPasswords)
{
    {
        Manager settings;
        SettingsManager::addConnection(connection("production", "Pl41nT3xtP4ss"));
        ASSERT_TRUE(settings->save());
    }

    QString const config = readConfigFile();

    EXPECT_FALSE(config.isEmpty());
    EXPECT_TRUE(config.contains("production.example.com"));
    EXPECT_FALSE(config.contains("Pl41nT3xtP4ss"));
}

TEST_F(SettingsManagerTests, findsConnectionsByUuid)
{
    Manager settings;
    ConnectionSettings *production = connection("production", "a");
    SettingsManager::addConnection(production);

    EXPECT_EQ(production, settings->getConnectionSettingsByUuid(QString("{production}")));
    EXPECT_EQ(production, settings->getConnectionSettingsByUuid(std::string("{production}")));
    EXPECT_EQ(nullptr, settings->getConnectionSettingsByUuid(QString("{missing}")));
}

TEST_F(SettingsManagerTests, removesAndReordersConnections)
{
    Manager settings;
    ConnectionSettings *a = connection("a", "1");
    ConnectionSettings *b = connection("b", "2");
    ConnectionSettings *c = connection("c", "3");
    SettingsManager::addConnection(a);
    SettingsManager::addConnection(b);
    SettingsManager::addConnection(c);

    settings->reorderConnections({ c, a, b });
    EXPECT_EQ(c, settings->connections()[0]);

    settings->removeConnection(a);
    ASSERT_EQ(2u, settings->connections().size());
    EXPECT_EQ(c, settings->connections()[0]);
    EXPECT_EQ(b, settings->connections()[1]);
}

TEST_F(SettingsManagerTests, outOfRangeValuesFallBackToDefaults)
{
    writeConfigFile(R"({ "version": "2.0", "uuidEncoding": 9, "timeZone": 5, "viewMode": -1,
                         "autocompletionMode": 7, "connections": [] })");

    Manager const settings;

    EXPECT_EQ(DefaultEncoding, settings->uuidEncoding());
    EXPECT_EQ(Utc, settings->timeZone());
    EXPECT_EQ(Custom, settings->viewMode());
    EXPECT_EQ(AutocompleteAll, settings->autocompletionMode());
}

TEST_F(SettingsManagerTests, nonPositiveFontSizeMeansDefault)
{
    Manager settings;

    settings->setTextFontPointSize(0);
    EXPECT_EQ(-1, settings->textFontPointSize());
    settings->setTextFontPointSize(-5);
    EXPECT_EQ(-1, settings->textFontPointSize());
}

TEST_F(SettingsManagerTests, keepsCacheData)
{
    Manager settings;

    settings->addCacheData("lastQuery", "db.users.find()");

    EXPECT_EQ(QVariant("db.users.find()"), settings->cacheData("lastQuery"));
    EXPECT_FALSE(settings->cacheData("missing").isValid());
}
