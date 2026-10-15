#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/core/domain/ScriptInfo.h"

using namespace Robomongo;

namespace
{
    QByteArray readBytes(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return QByteArray();
        return file.readAll();
    }
}

class ScriptInfoTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
};

TEST_F(ScriptInfoTests, keepsConstructorValues)
{
    ScriptInfo const info("db.users.find({})", true, "shop", CursorPosition(0, -2), "users", "/tmp/q.js");

    EXPECT_EQ(QString("db.users.find({})"), info.script());
    EXPECT_TRUE(info.execute());
    EXPECT_EQ("shop", info.dbname());
    EXPECT_EQ(0, info.cursor().line());
    EXPECT_EQ(-2, info.cursor().column());
    EXPECT_EQ(QString("users"), info.title());
    EXPECT_EQ(QString("/tmp/q.js"), info.filePath());
}

TEST_F(ScriptInfoTests, defaults)
{
    ScriptInfo const info("print(1)");

    EXPECT_FALSE(info.execute());
    EXPECT_EQ("", info.dbname());
    EXPECT_TRUE(info.cursor().isNull());
    EXPECT_TRUE(info.title().isEmpty());
    EXPECT_TRUE(info.filePath().isEmpty());
}

TEST_F(ScriptInfoTests, scriptAndExecutableCanChange)
{
    ScriptInfo info("old");

    info.setScript("new");
    info.setExecutable(true);

    EXPECT_EQ(QString("new"), info.script());
    EXPECT_TRUE(info.execute());
}

TEST_F(ScriptInfoTests, savesScriptToItsFile)
{
    QTemporaryDir dir;
    QString const path = dir.path() + "/query.js";
    ScriptInfo info("db.users.find({ age: { $gt: 21 } })\n", false, "", CursorPosition(), "", path);

    EXPECT_TRUE(info.saveToFile());

    EXPECT_EQ(QByteArray("db.users.find({ age: { $gt: 21 } })\n"), readBytes(path));
}

TEST_F(ScriptInfoTests, savesUnicodeAsUtf8)
{
    QTemporaryDir dir;
    QString const path = dir.path() + "/unicode.js";
    ScriptInfo info(QString::fromUtf8("db.users.find({ name: \"Zoë 名前 🚀\" })"), false, "",
                    CursorPosition(), "", path);

    EXPECT_TRUE(info.saveToFile());

    EXPECT_EQ(QByteArray("db.users.find({ name: \"Zoë 名前 🚀\" })"), readBytes(path));
}

TEST_F(ScriptInfoTests, saveOverwritesExistingFile)
{
    QTemporaryDir dir;
    QString const path = dir.path() + "/query.js";
    ScriptInfo first("a much longer original script", false, "", CursorPosition(), "", path);
    ASSERT_TRUE(first.saveToFile());
    ScriptInfo second("short", false, "", CursorPosition(), "", path);

    EXPECT_TRUE(second.saveToFile());

    EXPECT_EQ(QByteArray("short"), readBytes(path));
}
