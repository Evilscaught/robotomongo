#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include "robomongo/utils/RoboCrypt.h"

using namespace Robomongo;

namespace
{
    // Points QDir::homePath() at a temporary directory so the real ~/.3T is never touched
    class TemporaryHome
    {
    public:
        TemporaryHome() : _previousHome(qgetenv("HOME"))
        {
            qputenv("HOME", _dir.path().toUtf8());
            QDir(_dir.path()).mkpath(".3T/robo-3t");
        }

        ~TemporaryHome() { qputenv("HOME", _previousHome); }

        QString keyFilePath() const { return _dir.path() + "/.3T/robo-3t/robo3t.key"; }

        QString readKeyFile() const
        {
            QFile file(keyFilePath());
            if (!file.open(QIODevice::ReadOnly))
                return QString();
            return QTextStream(&file).readAll();
        }

        void writeKeyFile(const QString &content) const
        {
            QFile file(keyFilePath());
            ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
            QTextStream(&file) << content;
        }

    private:
        QTemporaryDir _dir;
        QByteArray const _previousHome;
    };

    size_t logCount() { return RoboCrypt::roboCryptLogs().size(); }

    bool logsSince(size_t start, const std::string &text)
    {
        auto const& logs = RoboCrypt::roboCryptLogs();
        for (size_t i = start; i < logs.size(); ++i) {
            if (logs[i].first.find(text) != std::string::npos)
                return true;
        }
        return false;
    }
}

TEST(RoboCryptKeyTests, temporaryHomeIsUsed)
{
    TemporaryHome home;

    EXPECT_TRUE(home.keyFilePath().startsWith(QDir::homePath()));
    EXPECT_FALSE(QFile::exists(home.keyFilePath()));
}

TEST(RoboCryptKeyTests, generatesAndSavesKeyWhenMissing)
{
    TemporaryHome home;
    size_t const before = logCount();

    RoboCrypt::initKey();

    QString const saved = home.readKeyFile();
    bool isNumber = false;
    qlonglong const key = saved.toLongLong(&isNumber);
    ASSERT_TRUE(isNumber) << saved.toStdString();
    EXPECT_GE(key, 1LL << 61);
    EXPECT_LE(key, 1LL << 62);
    EXPECT_TRUE(logsSince(before, "generating a new key"));
}

TEST(RoboCryptKeyTests, generatedKeysDiffer)
{
    TemporaryHome first;
    RoboCrypt::initKey();
    QString const firstKey = first.readKeyFile();

    TemporaryHome second;
    RoboCrypt::initKey();
    QString const secondKey = second.readKeyFile();

    EXPECT_NE(firstKey, secondKey);
}

TEST(RoboCryptKeyTests, keepsExistingKeyFile)
{
    TemporaryHome home;
    home.writeKeyFile("3000000000000000000");
    size_t const before = logCount();

    RoboCrypt::initKey();

    EXPECT_EQ(QString("3000000000000000000"), home.readKeyFile());
    EXPECT_FALSE(logsSince(before, "generating a new key"));
    EXPECT_FALSE(logsSince(before, "Key file is empty"));
}

TEST(RoboCryptKeyTests, logsEmptyKeyFile)
{
    TemporaryHome home;
    home.writeKeyFile("");
    size_t const before = logCount();

    RoboCrypt::initKey();

    EXPECT_TRUE(logsSince(before, "Key file is empty"));
}

TEST(RoboCryptKeyTests, logsWhenKeyCannotBeSaved)
{
    TemporaryHome home;
    QDir(QFileInfo(home.keyFilePath()).path()).removeRecursively();
    size_t const before = logCount();

    RoboCrypt::initKey();

    EXPECT_TRUE(logsSince(before, "Failed to save the key"));
}

TEST(RoboCryptTests, roundTripsUnicodeAndLongPasswords)
{
    for (std::string const& password : { std::string("pässwörd"), std::string("пароль密码"),
                                         std::string(500, 'x'), std::string("with space and\ttab") }) {
        EXPECT_EQ(password, RoboCrypt::decrypt(RoboCrypt::encrypt(password)));
    }
}

TEST(RoboCryptTests, encryptedPasswordDiffersFromPlaintext)
{
    std::string const encrypted = RoboCrypt::encrypt("correct horse battery staple");

    EXPECT_NE("correct horse battery staple", encrypted);
    EXPECT_EQ(std::string::npos, encrypted.find("horse"));
}

TEST(RoboCryptTests, decryptingGarbageGivesEmptyPassword)
{
    EXPECT_EQ("", RoboCrypt::decrypt("not encrypted at all"));
}
