#include <gtest/gtest.h>

#include "robomongo/utils/SimpleCrypt.h"

namespace
{
    const quint64 Key = Q_UINT64_C(0x0c2ad4a4acb9f023);
    const quint64 OtherKey = Q_UINT64_C(0x1d3be5b5bdca0134);
}

TEST(SimpleCryptTests, roundTripsText)
{
    SimpleCrypt crypt(Key);

    QString const encrypted = crypt.encryptToString(QString("p@ssw0rd!"));

    EXPECT_EQ(QString("p@ssw0rd!"), crypt.decryptToString(encrypted));
    EXPECT_EQ(SimpleCrypt::ErrorNoError, crypt.lastError());
}

TEST(SimpleCryptTests, ciphertextDoesNotContainPlaintext)
{
    SimpleCrypt crypt(Key);

    QString const encrypted = crypt.encryptToString(QString("hunter2-hunter2"));

    EXPECT_FALSE(encrypted.contains("hunter2"));
    EXPECT_NE(QString("hunter2-hunter2"), encrypted);
}

TEST(SimpleCryptTests, roundTripsUnicodeAndEmptyText)
{
    SimpleCrypt crypt(Key);

    for (QString const& text : { QString(""), QString("contraseña"), QString::fromUtf8("пароль密码🔑") }) {
        QString const encrypted = crypt.encryptToString(text);
        EXPECT_EQ(text, crypt.decryptToString(encrypted)) << text.toStdString();
    }
}

TEST(SimpleCryptTests, roundTripsBinaryData)
{
    SimpleCrypt crypt(Key);
    QByteArray binary;
    for (int i = 0; i < 256; ++i)
        binary.append(char(i));

    QByteArray const encrypted = crypt.encryptToByteArray(binary);

    EXPECT_EQ(binary, crypt.decryptToByteArray(encrypted));
}

TEST(SimpleCryptTests, roundTripsLongTextWithEveryCompressionMode)
{
    QString const longText = QString("repetitive password text ").repeated(200);

    for (auto mode : { SimpleCrypt::CompressionAuto, SimpleCrypt::CompressionAlways,
                       SimpleCrypt::CompressionNever }) {
        SimpleCrypt crypt(Key);
        crypt.setCompressionMode(mode);
        QString const encrypted = crypt.encryptToString(longText);
        EXPECT_EQ(longText, crypt.decryptToString(encrypted)) << mode;
    }
}

TEST(SimpleCryptTests, autoCompressionShrinksRepetitiveText)
{
    QString const longText = QString("aaaaaaaaaa").repeated(500);
    SimpleCrypt compressing(Key);
    SimpleCrypt plain(Key);
    plain.setCompressionMode(SimpleCrypt::CompressionNever);

    EXPECT_LT(compressing.encryptToByteArray(longText).size(), plain.encryptToByteArray(longText).size());
}

TEST(SimpleCryptTests, roundTripsWithEveryIntegrityMode)
{
    for (auto mode : { SimpleCrypt::ProtectionNone, SimpleCrypt::ProtectionChecksum,
                       SimpleCrypt::ProtectionHash }) {
        SimpleCrypt crypt(Key);
        crypt.setIntegrityProtectionMode(mode);
        QString const encrypted = crypt.encryptToString(QString("secret"));
        EXPECT_EQ(QString("secret"), crypt.decryptToString(encrypted)) << mode;
    }
}

TEST(SimpleCryptTests, wrongKeyFailsIntegrityCheck)
{
    SimpleCrypt writer(Key);
    writer.setIntegrityProtectionMode(SimpleCrypt::ProtectionHash);
    SimpleCrypt reader(OtherKey);
    reader.setIntegrityProtectionMode(SimpleCrypt::ProtectionHash);

    QString const decrypted = reader.decryptToString(writer.encryptToString(QString("secret")));

    EXPECT_TRUE(decrypted.isEmpty());
    EXPECT_EQ(SimpleCrypt::ErrorIntegrityFailed, reader.lastError());
}

TEST(SimpleCryptTests, tamperedCiphertextFailsIntegrityCheck)
{
    SimpleCrypt crypt(Key);
    crypt.setIntegrityProtectionMode(SimpleCrypt::ProtectionChecksum);
    QByteArray encrypted = crypt.encryptToByteArray(QString("secret"));

    encrypted[encrypted.size() - 1] = encrypted.at(encrypted.size() - 1) ^ 0x5a;

    EXPECT_TRUE(crypt.decryptToByteArray(encrypted).isEmpty());
    EXPECT_EQ(SimpleCrypt::ErrorIntegrityFailed, crypt.lastError());
}

TEST(SimpleCryptTests, rejectsUnknownVersion)
{
    SimpleCrypt crypt(Key);
    QByteArray encrypted = crypt.encryptToByteArray(QString("secret"));

    encrypted[0] = char(0x02);

    EXPECT_TRUE(crypt.decryptToByteArray(encrypted).isEmpty());
    EXPECT_EQ(SimpleCrypt::ErrorUnknownVersion, crypt.lastError());
}

TEST(SimpleCryptTests, requiresKey)
{
    SimpleCrypt crypt;

    EXPECT_TRUE(crypt.encryptToString(QString("secret")).isEmpty());
    EXPECT_EQ(SimpleCrypt::ErrorNoKeySet, crypt.lastError());
    EXPECT_TRUE(crypt.decryptToString(QString("anything")).isEmpty());
    EXPECT_EQ(SimpleCrypt::ErrorNoKeySet, crypt.lastError());
}

TEST(SimpleCryptTests, setKeyEnablesEncryption)
{
    SimpleCrypt crypt;
    crypt.setKey(Key);

    QString const encrypted = crypt.encryptToString(QString("secret"));

    EXPECT_EQ(QString("secret"), SimpleCrypt(Key).decryptToString(encrypted));
}

TEST(SimpleCryptTests, differentKeysProduceDifferentCiphertext)
{
    SimpleCrypt first(Key);
    SimpleCrypt second(OtherKey);
    first.setCompressionMode(SimpleCrypt::CompressionNever);
    second.setCompressionMode(SimpleCrypt::CompressionNever);

    // Skip the version/flags header and the random first byte
    QByteArray const a = first.encryptToByteArray(QString("same plaintext")).mid(3);
    QByteArray const b = second.encryptToByteArray(QString("same plaintext")).mid(3);

    EXPECT_NE(a, b);
}

TEST(SimpleCryptTests, ciphertextStartsWithVersionThree)
{
    SimpleCrypt crypt(Key);

    QByteArray const encrypted = crypt.encryptToByteArray(QString("secret"));

    ASSERT_GE(encrypted.size(), 3);
    EXPECT_EQ(char(0x03), encrypted.at(0));
}
