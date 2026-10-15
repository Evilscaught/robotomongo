#include <gtest/gtest.h>

#include <QFile>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QTreeWidgetItem>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo/core/Enums.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/utils/StringOperations.h"
#include "robomongo/utils/common.h"

using namespace Robomongo;

// ---- QtUtils ----

TEST(QtUtilsTests, convertsUtf8StdStringToQString)
{
    EXPECT_EQ(QString::fromUtf8("héllo 🚀"), QtUtils::toQString(std::string("héllo 🚀")));
    EXPECT_EQ(QString(""), QtUtils::toQString(std::string()));
}

TEST(QtUtilsTests, keepsEmbeddedNulCharacters)
{
    std::string const withNul("a\0b", 3);

    QString const converted = QtUtils::toQString(withNul);

    EXPECT_EQ(3, converted.size());
    EXPECT_EQ(withNul, QtUtils::toStdString(converted));
}

TEST(QtUtilsTests, convertsQStringToUtf8StdString)
{
    EXPECT_EQ("héllo 🚀", QtUtils::toStdString(QString::fromUtf8("héllo 🚀")));
    EXPECT_EQ("", QtUtils::toStdString(QString()));
}

TEST(QtUtilsTests, stringConversionsRoundTrip)
{
    for (std::string const& text : { std::string("plain"), std::string("ünïcødé"),
                                     std::string("日本語のテキスト"), std::string("emoji 🎉🔑") }) {
        EXPECT_EQ(text, QtUtils::toStdString(QtUtils::toQString(text)));
        EXPECT_EQ(text, QtUtils::toStdStringSafe(QtUtils::toQString(text)));
    }
}

TEST(QtUtilsTests, clearChildItemsRemovesAllChildren)
{
    QTreeWidgetItem root;
    for (int i = 0; i < 5; ++i)
        new QTreeWidgetItem(&root, QStringList(QString::number(i)));
    QTreeWidgetItem *grandChild = new QTreeWidgetItem(root.child(0));
    (void)grandChild;

    QtUtils::clearChildItems(&root);

    EXPECT_EQ(0, root.childCount());
}

TEST(QtUtilsTests, clearChildItemsOnLeafDoesNothing)
{
    QTreeWidgetItem leaf;

    QtUtils::clearChildItems(&leaf);

    EXPECT_EQ(0, leaf.childCount());
}

TEST(QtUtilsTests, itemReturnsInternalPointer)
{
    int value = 42;
    QStandardItemModel model;

    struct Model : QStandardItemModel
    {
        QModelIndex make(int *pointer) { return createIndex(0, 0, pointer); }
    } indexFactory;

    QModelIndex const index = indexFactory.make(&value);

    EXPECT_EQ(&value, QtUtils::item<int *>(index));
    EXPECT_EQ(nullptr, QtUtils::item<int *>(QModelIndex()));
}

// ---- StringOperations ----

TEST(StringOperationsTests, capitalisesOnlyFirstCharacter)
{
    EXPECT_EQ("Hello world", captilizeFirstChar("hello world"));
    EXPECT_EQ("ALREADY", captilizeFirstChar("ALREADY"));
    EXPECT_EQ("X", captilizeFirstChar("x"));
}

TEST(StringOperationsTests, leavesNonLettersAndEmptyStringsAlone)
{
    EXPECT_EQ("", captilizeFirstChar(""));
    EXPECT_EQ("123abc", captilizeFirstChar("123abc"));
    EXPECT_EQ(" lead", captilizeFirstChar(" lead"));
}

// ---- common ----

TEST(CommonUtilsTests, fileExistsOnlyForRegularFiles)
{
    QTemporaryDir dir;
    QString const filePath = dir.path() + "/file.txt";
    QFile file(filePath);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("x");
    file.close();

    EXPECT_TRUE(fileExists(filePath));
    EXPECT_FALSE(fileExists(dir.path()));
    EXPECT_FALSE(fileExists(dir.path() + "/missing.txt"));
    EXPECT_FALSE(fileExists(""));
}

// ---- Enums ----

TEST(EnumsTests, uuidEncodingNamesRoundTrip)
{
    for (UUIDEncoding encoding : { DefaultEncoding, JavaLegacy, CSharpLegacy, PythonLegacy })
        EXPECT_EQ(encoding, convertStringToUUIDEncoding(convertUUIDEncodingToString(encoding)));

    EXPECT_STREQ("Default encoding", convertUUIDEncodingToString(DefaultEncoding));
    EXPECT_STREQ("Java encoding", convertUUIDEncodingToString(JavaLegacy));
    EXPECT_STREQ("CSharp encoding", convertUUIDEncodingToString(CSharpLegacy));
    EXPECT_STREQ("Python encoding", convertUUIDEncodingToString(PythonLegacy));
}

TEST(EnumsTests, timeZoneNamesRoundTrip)
{
    EXPECT_STREQ("UTC", convertTimesToString(Utc));
    EXPECT_STREQ("Local Timezone", convertTimesToString(LocalTime));
    EXPECT_EQ(Utc, convertStringToTimes("UTC"));
    EXPECT_EQ(LocalTime, convertStringToTimes("Local Timezone"));
}

TEST(EnumsTests, viewModeNamesRoundTrip)
{
    for (ViewMode mode : { Text, Tree, Table, Custom })
        EXPECT_EQ(mode, convertStringToViewMode(convertViewModeToString(mode)));

    EXPECT_STREQ("Tree mode", convertViewModeToString(Tree));
}

TEST(EnumsTests, unknownNamesFallBackToFirstValue)
{
    EXPECT_EQ(DefaultEncoding, convertStringToUUIDEncoding("Nonsense"));
    EXPECT_EQ(Utc, convertStringToTimes(""));
    EXPECT_EQ(Text, convertStringToViewMode("tree mode"));
}
