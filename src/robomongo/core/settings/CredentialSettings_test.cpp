#include <gtest/gtest.h>

#include <memory>

#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/utils/RoboCrypt.h"

using namespace Robomongo;

namespace
{
    CredentialSettings fullCredential()
    {
        CredentialSettings credential;
        credential.setUserName("ada");
        credential.setUserPassword("s3cr3t:p@ss");
        credential.setDatabaseName("shop");
        credential.setMechanism("SCRAM-SHA-256");
        credential.setEnabled(true);
        credential.setUseManuallyVisibleDbs(true);
        credential.setManuallyVisibleDbs("shop,logs");
        return credential;
    }
}

TEST(CredentialSettingsTests, defaults)
{
    CredentialSettings const credential;

    EXPECT_EQ("", credential.userName());
    EXPECT_EQ("", credential.userPassword());
    EXPECT_EQ("admin", credential.databaseName());
    EXPECT_EQ("SCRAM-SHA-1", credential.mechanism());
    EXPECT_FALSE(credential.enabled());
    EXPECT_FALSE(credential.useManuallyVisibleDbs());
    EXPECT_EQ("", credential.manuallyVisibleDbs());
}

TEST(CredentialSettingsTests, emptyDatabaseAndMechanismFallBackToDefaults)
{
    CredentialSettings credential;
    credential.setDatabaseName("");
    credential.setMechanism("");

    EXPECT_EQ("admin", credential.databaseName());
    EXPECT_EQ("SCRAM-SHA-1", credential.mechanism());
}

TEST(CredentialSettingsTests, roundTripsThroughVariant)
{
    CredentialSettings const original = fullCredential();

    CredentialSettings const loaded(original.toVariant().toMap());

    EXPECT_EQ("ada", loaded.userName());
    EXPECT_EQ("s3cr3t:p@ss", loaded.userPassword());
    EXPECT_EQ("shop", loaded.databaseName());
    EXPECT_EQ("SCRAM-SHA-256", loaded.mechanism());
    EXPECT_TRUE(loaded.enabled());
    EXPECT_TRUE(loaded.useManuallyVisibleDbs());
    EXPECT_EQ("shop,logs", loaded.manuallyVisibleDbs());
}

TEST(CredentialSettingsTests, passwordIsNeverSavedInPlainText)
{
    QVariantMap const map = fullCredential().toVariant().toMap();

    EXPECT_FALSE(map.contains("userPassword"));
    ASSERT_TRUE(map.contains("userPasswordEncrypted"));
    QString const encrypted = map.value("userPasswordEncrypted").toString();
    EXPECT_FALSE(encrypted.isEmpty());
    EXPECT_FALSE(encrypted.contains("s3cr3t"));
    for (QVariant const& value : map)
        EXPECT_FALSE(value.toString().contains("s3cr3t"));
}

TEST(CredentialSettingsTests, emptyPasswordIsSavedEmpty)
{
    CredentialSettings credential;
    credential.setUserName("ada");

    QVariantMap const map = credential.toVariant().toMap();

    EXPECT_EQ(QString(""), map.value("userPasswordEncrypted").toString());
    EXPECT_EQ("", CredentialSettings(map).userPassword());
}

TEST(CredentialSettingsTests, readsLegacyPlainTextPassword)
{
    // Robo 3T 1.2 and earlier saved passwords as "userPassword"
    QVariantMap map;
    map.insert("userName", "ada");
    map.insert("userPassword", "legacy-pass");

    EXPECT_EQ("legacy-pass", CredentialSettings(map).userPassword());
}

TEST(CredentialSettingsTests, legacyPasswordTakesPrecedence)
{
    QVariantMap map;
    map.insert("userPassword", "legacy-pass");
    map.insert("userPasswordEncrypted", QString::fromStdString(RoboCrypt::encrypt("new-pass")));

    EXPECT_EQ("legacy-pass", CredentialSettings(map).userPassword());
}

TEST(CredentialSettingsTests, readsEncryptedPassword)
{
    QVariantMap map;
    map.insert("userPasswordEncrypted", QString::fromStdString(RoboCrypt::encrypt("pässwörd")));

    EXPECT_EQ("pässwörd", CredentialSettings(map).userPassword());
}

TEST(CredentialSettingsTests, missingFieldsUseDefaults)
{
    CredentialSettings const credential{ QVariantMap() };

    EXPECT_EQ("", credential.userName());
    EXPECT_EQ("", credential.userPassword());
    EXPECT_EQ("admin", credential.databaseName());
    EXPECT_EQ("SCRAM-SHA-1", credential.mechanism());
    EXPECT_FALSE(credential.enabled());
}

TEST(CredentialSettingsTests, cloneIsIndependentCopy)
{
    CredentialSettings original = fullCredential();
    std::unique_ptr<CredentialSettings> const copy(original.clone());

    original.setUserPassword("changed");
    original.setEnabled(false);

    EXPECT_EQ("s3cr3t:p@ss", copy->userPassword());
    EXPECT_TRUE(copy->enabled());
    EXPECT_EQ("ada", copy->userName());
}
