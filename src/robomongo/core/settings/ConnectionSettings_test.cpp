#include <gtest/gtest.h>

#include <memory>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"

using namespace Robomongo;

namespace
{
    CredentialSettings *credential(const std::string &db, const std::string &user, bool enabled = true)
    {
        auto cred = new CredentialSettings();
        cred->setDatabaseName(db);
        cred->setUserName(user);
        cred->setUserPassword(user + "-pass");
        cred->setEnabled(enabled);
        return cred;
    }

    std::unique_ptr<ConnectionSettings> fullConnection()
    {
        std::unique_ptr<ConnectionSettings> settings(new ConnectionSettings(false));
        settings->setConnectionName("Production");
        settings->setServerHost("db.example.com");
        settings->setServerPort(27018);
        settings->setDefaultDatabase("shop");
        settings->addCredential(credential("admin", "root"));
        settings->addCredential(credential("shop", "app", false));
        settings->sshSettings()->setEnabled(true);
        settings->sshSettings()->setHost("bastion");
        settings->sslSettings()->enableSSL(true);
        settings->setUuid("{11111111-2222-3333-4444-555555555555}");
        return settings;
    }
}

TEST(ConnectionSettingsTests, defaults)
{
    ConnectionSettings const settings(false);

    EXPECT_EQ("New Connection", settings.connectionName());
    EXPECT_EQ("localhost", settings.serverHost());
    EXPECT_EQ(27017, settings.serverPort());
    EXPECT_EQ("", settings.defaultDatabase());
    EXPECT_FALSE(settings.imported());
    EXPECT_FALSE(settings.isReplicaSet());
    EXPECT_EQ(0, settings.credentialCount());
    EXPECT_FALSE(settings.sshSettings()->enabled());
    EXPECT_FALSE(settings.sslSettings()->sslEnabled());
}

TEST(ConnectionSettingsTests, roundTripsThroughVariant)
{
    auto const original = fullConnection();

    ConnectionSettings loaded(false);
    loaded.fromVariant(original->toVariant().toMap());

    EXPECT_EQ("Production", loaded.connectionName());
    EXPECT_EQ("db.example.com", loaded.serverHost());
    EXPECT_EQ(27018, loaded.serverPort());
    EXPECT_EQ("shop", loaded.defaultDatabase());
    EXPECT_EQ(original->uuid(), loaded.uuid());
    ASSERT_EQ(2, loaded.credentialCount());
    EXPECT_EQ("root", loaded.primaryCredential()->userName());
    EXPECT_EQ("root-pass", loaded.primaryCredential()->userPassword());
    EXPECT_TRUE(loaded.sshSettings()->enabled());
    EXPECT_EQ("bastion", loaded.sshSettings()->host());
    EXPECT_TRUE(loaded.sslSettings()->sslEnabled());
}

TEST(ConnectionSettingsTests, savedVariantHasNoPlainTextPasswords)
{
    QVariantMap const map = fullConnection()->toVariant().toMap();

    for (QVariant const& credential : map.value("credentials").toList()) {
        QVariantMap const fields = credential.toMap();
        EXPECT_FALSE(fields.contains("userPassword"));
        for (QVariant const& value : fields)
            EXPECT_FALSE(value.toString().contains("-pass"));
    }
}

TEST(ConnectionSettingsTests, createsUuidWhenMissing)
{
    QVariantMap map = fullConnection()->toVariant().toMap();
    map.remove("uuid");

    ConnectionSettings loaded(false);
    loaded.fromVariant(map);

    EXPECT_FALSE(loaded.uuid().isEmpty());
    EXPECT_TRUE(loaded.uuid().startsWith("{"));
}

TEST(ConnectionSettingsTests, createsUuidWhenEmpty)
{
    QVariantMap map = fullConnection()->toVariant().toMap();
    map.insert("uuid", "");

    ConnectionSettings first(false);
    first.fromVariant(map);
    ConnectionSettings second(false);
    second.fromVariant(map);

    EXPECT_FALSE(first.uuid().isEmpty());
    EXPECT_NE(first.uuid(), second.uuid());
}

TEST(ConnectionSettingsTests, truncatesOverlongHost)
{
    QVariantMap map = fullConnection()->toVariant().toMap();
    map.insert("serverHost", QString(500, 'h'));

    ConnectionSettings loaded(false);
    loaded.fromVariant(map);

    EXPECT_EQ(300u, loaded.serverHost().size());
}

TEST(ConnectionSettingsTests, replicaSetDetailsOnlySavedForReplicaSets)
{
    auto settings = fullConnection();
    settings->replicaSetSettings()->setCachedSetName("rs0");

    EXPECT_FALSE(settings->toVariant().toMap().contains("replicaSet"));

    settings->setReplicaSet(true);
    QVariantMap const map = settings->toVariant().toMap();
    ASSERT_TRUE(map.contains("replicaSet"));

    ConnectionSettings loaded(false);
    loaded.fromVariant(map);
    EXPECT_TRUE(loaded.isReplicaSet());
    EXPECT_EQ("rs0", loaded.replicaSetSettings()->cachedSetName());
}

TEST(ConnectionSettingsTests, ignoresDuplicateCredentialForSameDatabase)
{
    ConnectionSettings settings(false);
    settings.addCredential(credential("shop", "first"));

    CredentialSettings *duplicate = credential("shop", "second");
    settings.addCredential(duplicate);

    EXPECT_EQ(1, settings.credentialCount());
    EXPECT_EQ("first", settings.primaryCredential()->userName());
    delete duplicate;   // not taken over by settings
}

TEST(ConnectionSettingsTests, primaryCredentialIsFirst)
{
    auto const settings = fullConnection();

    EXPECT_EQ("root", settings->primaryCredential()->userName());
    EXPECT_TRUE(settings->hasEnabledPrimaryCredential());
}

TEST(ConnectionSettingsTests, primaryCredentialCreatedOnDemand)
{
    ConnectionSettings const settings(false);

    EXPECT_FALSE(settings.hasEnabledPrimaryCredential());
    CredentialSettings const *primary = settings.primaryCredential();

    ASSERT_NE(nullptr, primary);
    EXPECT_EQ(1, settings.credentialCount());
    EXPECT_FALSE(primary->enabled());
    EXPECT_FALSE(settings.hasEnabledPrimaryCredential());
}

TEST(ConnectionSettingsTests, disabledPrimaryCredentialIsNotUsed)
{
    ConnectionSettings settings(false);
    settings.addCredential(credential("admin", "root", false));

    EXPECT_FALSE(settings.hasEnabledPrimaryCredential());
}

TEST(ConnectionSettingsTests, clearCredentials)
{
    auto settings = fullConnection();

    settings->clearCredentials();

    EXPECT_EQ(0, settings->credentialCount());
}

TEST(ConnectionSettingsTests, cloneIsDeepCopy)
{
    auto original = fullConnection();
    std::unique_ptr<ConnectionSettings> const copy(original->clone());

    original->setServerHost("changed");
    original->primaryCredential()->setUserPassword("changed");
    original->sshSettings()->setHost("changed");
    original->sslSettings()->enableSSL(false);

    EXPECT_EQ("db.example.com", copy->serverHost());
    EXPECT_EQ("root-pass", copy->primaryCredential()->userPassword());
    EXPECT_EQ("bastion", copy->sshSettings()->host());
    EXPECT_TRUE(copy->sslSettings()->sslEnabled());
    EXPECT_EQ(original->uuid(), copy->uuid());
}

TEST(ConnectionSettingsTests, applyReplacesStateButKeepsUuid)
{
    auto const source = fullConnection();
    ConnectionSettings target(false);
    target.setUuid("{target}");
    target.addCredential(credential("old", "old-user"));

    target.apply(source.get());

    EXPECT_EQ("Production", target.connectionName());
    EXPECT_EQ("db.example.com", target.serverHost());
    EXPECT_EQ(2, target.credentialCount());
    EXPECT_EQ("root", target.primaryCredential()->userName());
    EXPECT_EQ(QString("{target}"), target.uuid());
}

TEST(ConnectionSettingsTests, fullAddressForHostname)
{
    auto const settings = fullConnection();

    EXPECT_EQ("db.example.com:27018", settings->getFullAddress());
}

TEST(ConnectionSettingsTests, fullAddressForIpv4)
{
    ConnectionSettings settings(false);
    settings.setServerHost("10.0.0.5");

    EXPECT_EQ("10.0.0.5:27017", settings.getFullAddress());
}

TEST(ConnectionSettingsTests, hostAndPortForIpv6WithAndWithoutBrackets)
{
    ConnectionSettings bare(false);
    bare.setServerHost("::1");
    ConnectionSettings bracketed(false);
    bracketed.setServerHost("[::1]");

    EXPECT_EQ("::1", bare.hostAndPort().host());
    EXPECT_EQ("::1", bracketed.hostAndPort().host());
    EXPECT_EQ(27017, bare.hostAndPort().port());
    EXPECT_EQ(bare.getFullAddress(), bracketed.getFullAddress());
}

TEST(ConnectionSettingsTests, readableNameFallsBackToAddress)
{
    ConnectionSettings settings(false);
    settings.setConnectionName("");
    settings.setServerHost("db.example.com");

    EXPECT_EQ("db.example.com:27017", settings.getReadableName());

    settings.setConnectionName("Staging");
    EXPECT_EQ("Staging", settings.getReadableName());
}
