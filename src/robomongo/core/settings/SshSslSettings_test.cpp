#include <gtest/gtest.h>

#include <memory>

#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/utils/RoboCrypt.h"

using namespace Robomongo;

namespace
{
    SshSettings fullSsh()
    {
        SshSettings ssh;
        ssh.setHost("bastion.example.com");
        ssh.setPort(2222);
        ssh.setUserName("deploy");
        ssh.setUserPassword("ssh-pass");
        ssh.setPrivateKeyFile("/home/deploy/.ssh/id_ed25519");
        ssh.setPublicKeyFile("/home/deploy/.ssh/id_ed25519.pub");
        ssh.setPassphrase("key-passphrase");
        ssh.setAuthMethod("password");
        ssh.setEnabled(true);
        ssh.setAskPassword(true);
        return ssh;
    }

    SslSettings fullSsl()
    {
        SslSettings ssl;
        ssl.enableSSL(true);
        ssl.setCaFile("/etc/ssl/ca.pem");
        ssl.setUsePemFile(true);
        ssl.setPemKeyFile("/etc/ssl/client.pem");
        ssl.setPemPassPhrase("pem-pass");
        ssl.setUseAdvancedOptions(true);
        ssl.setCrlFile("/etc/ssl/crl.pem");
        ssl.setAllowInvalidHostnames(true);
        ssl.setAllowInvalidCertificates(true);
        ssl.setAskPassphrase(true);
        return ssl;
    }

    bool mapContainsText(const QVariantMap &map, const QString &text)
    {
        for (QVariant const& value : map) {
            if (value.toString().contains(text))
                return true;
        }
        return false;
    }
}

TEST(SshSettingsTests, defaults)
{
    SshSettings const ssh;

    EXPECT_EQ("", ssh.host());
    EXPECT_EQ(22, ssh.port());
    EXPECT_EQ("publickey", ssh.authMethod());
    EXPECT_FALSE(ssh.enabled());
    EXPECT_FALSE(ssh.askPassword());
    EXPECT_EQ(1, ssh.logLevel());
}

TEST(SshSettingsTests, emptyAuthMethodFallsBackToPublicKey)
{
    SshSettings ssh;
    ssh.setAuthMethod("");

    EXPECT_EQ("publickey", ssh.authMethod());
}

TEST(SshSettingsTests, roundTripsThroughVariant)
{
    SshSettings loaded;
    loaded.fromVariant(fullSsh().toVariant().toMap());

    EXPECT_EQ("bastion.example.com", loaded.host());
    EXPECT_EQ(2222, loaded.port());
    EXPECT_EQ("deploy", loaded.userName());
    EXPECT_EQ("ssh-pass", loaded.userPassword());
    EXPECT_EQ("/home/deploy/.ssh/id_ed25519", loaded.privateKeyFile());
    EXPECT_EQ("/home/deploy/.ssh/id_ed25519.pub", loaded.publicKeyFile());
    EXPECT_EQ("key-passphrase", loaded.passphrase());
    EXPECT_EQ("password", loaded.authMethod());
    EXPECT_TRUE(loaded.enabled());
    EXPECT_TRUE(loaded.askPassword());
}

TEST(SshSettingsTests, secretsAreNeverSavedInPlainText)
{
    QVariantMap const map = fullSsh().toVariant().toMap();

    EXPECT_FALSE(map.contains("userPassword"));
    EXPECT_FALSE(map.contains("passphrase"));
    EXPECT_FALSE(mapContainsText(map, "ssh-pass"));
    EXPECT_FALSE(mapContainsText(map, "key-passphrase"));
}

TEST(SshSettingsTests, transientValuesAreNotSaved)
{
    SshSettings ssh = fullSsh();
    ssh.setAskedPassword("typed-at-prompt");
    ssh.setLogLevel(4);

    QVariantMap const map = ssh.toVariant().toMap();

    EXPECT_FALSE(mapContainsText(map, "typed-at-prompt"));
    SshSettings loaded;
    loaded.fromVariant(map);
    EXPECT_EQ("", loaded.askedPassword());
    EXPECT_EQ(1, loaded.logLevel());
}

TEST(SshSettingsTests, emptySecretsAreSavedEmpty)
{
    SshSettings ssh;

    QVariantMap const map = ssh.toVariant().toMap();

    EXPECT_EQ(QString(""), map.value("userPasswordEncrypted").toString());
    EXPECT_EQ(QString(""), map.value("passphraseEncrypted").toString());
}

TEST(SshSettingsTests, readsLegacyPlainTextSecrets)
{
    QVariantMap map;
    map.insert("userPassword", "old-pass");
    map.insert("passphrase", "old-phrase");

    SshSettings ssh;
    ssh.fromVariant(map);

    EXPECT_EQ("old-pass", ssh.userPassword());
    EXPECT_EQ("old-phrase", ssh.passphrase());
}

TEST(SshSettingsTests, cloneIsIndependentCopy)
{
    SshSettings original = fullSsh();
    std::unique_ptr<SshSettings> const copy(original.clone());

    original.setHost("other");
    original.setUserPassword("changed");

    EXPECT_EQ("bastion.example.com", copy->host());
    EXPECT_EQ("ssh-pass", copy->userPassword());
}

TEST(SslSettingsTests, defaults)
{
    SslSettings const ssl;

    EXPECT_FALSE(ssl.sslEnabled());
    EXPECT_EQ("", ssl.caFile());
    EXPECT_EQ("", ssl.pemKeyFile());
    EXPECT_EQ("", ssl.pemPassPhrase());
    EXPECT_EQ("", ssl.crlFile());
    EXPECT_FALSE(ssl.allowInvalidHostnames());
    EXPECT_FALSE(ssl.allowInvalidCertificates());
    EXPECT_FALSE(ssl.usePemFile());
    EXPECT_FALSE(ssl.useAdvancedOptions());
    EXPECT_FALSE(ssl.askPassphrase());
}

TEST(SslSettingsTests, roundTripsThroughVariant)
{
    SslSettings loaded;
    loaded.fromVariant(fullSsl().toVariant().toMap());

    EXPECT_TRUE(loaded.sslEnabled());
    EXPECT_EQ("/etc/ssl/ca.pem", loaded.caFile());
    EXPECT_TRUE(loaded.usePemFile());
    EXPECT_EQ("/etc/ssl/client.pem", loaded.pemKeyFile());
    EXPECT_EQ("pem-pass", loaded.pemPassPhrase());
    EXPECT_TRUE(loaded.useAdvancedOptions());
    EXPECT_EQ("/etc/ssl/crl.pem", loaded.crlFile());
    EXPECT_TRUE(loaded.allowInvalidHostnames());
    EXPECT_TRUE(loaded.allowInvalidCertificates());
    EXPECT_TRUE(loaded.askPassphrase());
}

TEST(SslSettingsTests, passphraseIsNeverSavedInPlainText)
{
    QVariantMap const map = fullSsl().toVariant().toMap();

    EXPECT_FALSE(map.contains("pemPassPhrase"));
    EXPECT_FALSE(mapContainsText(map, "pem-pass"));
    EXPECT_FALSE(map.value("pemPassPhraseEncrypted").toString().isEmpty());
}

TEST(SslSettingsTests, readsLegacyPlainTextPassphrase)
{
    QVariantMap map;
    map.insert("pemPassPhrase", "old-phrase");

    SslSettings ssl;
    ssl.fromVariant(map);

    EXPECT_EQ("old-phrase", ssl.pemPassPhrase());
}

TEST(SslSettingsTests, insecureOptionsAreOffWhenMissingFromConfig)
{
    SslSettings ssl;
    ssl.fromVariant(QVariantMap{ { "sslEnabled", true } });

    EXPECT_TRUE(ssl.sslEnabled());
    EXPECT_FALSE(ssl.allowInvalidHostnames());
    EXPECT_FALSE(ssl.allowInvalidCertificates());
}

TEST(SslSettingsTests, cloneIsIndependentCopy)
{
    SslSettings original = fullSsl();
    std::unique_ptr<SslSettings> const copy(original.clone());

    original.enableSSL(false);
    original.setAllowInvalidCertificates(false);

    EXPECT_TRUE(copy->sslEnabled());
    EXPECT_TRUE(copy->allowInvalidCertificates());
}
