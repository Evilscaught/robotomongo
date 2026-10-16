// Runs in robo_isolated_tests (HOME is a scratch directory; see IsolatedMain.cpp)
//
// Only valid input is used: on a parse error validate() opens a modal message box. accept(),
// reject() and close() are not called because they write the window size to QSettings.

#include <gtest/gtest.h>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"

using namespace Robomongo;

namespace
{
    CollectionInfo const users("localhost:27017", "shop", "users");
}

class DocumentTextEditorTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
};

TEST_F(DocumentTextEditorTests, showsGivenJsonTrimmed)
{
    DocumentTextEditor editor(users, "\n  { \"name\" : \"Ada\" }  \n");

    EXPECT_EQ(QString("{ \"name\" : \"Ada\" }"), editor.jsonText());
}

TEST_F(DocumentTextEditorTests, validDocumentParses)
{
    DocumentTextEditor editor(users, "{ \"_id\" : ObjectId(\"5f1d7a2b3c4d5e6f70819203\"), \"name\" : \"Ada\" }");

    ASSERT_TRUE(editor.validate());

    ASSERT_EQ(1u, editor.bsonObj().size());
    EXPECT_EQ(std::string("Ada"), editor.bsonObj()[0].getStringField("name"));
    EXPECT_EQ("5f1d7a2b3c4d5e6f70819203", editor.bsonObj()[0].getField("_id").OID().toString());
}

TEST_F(DocumentTextEditorTests, severalDocumentsParseInOrder)
{
    DocumentTextEditor editor(users, "{ a: 1 }\n{ b: 2 }\n\n  { c: 3 }");

    ASSERT_TRUE(editor.validate());

    ASSERT_EQ(3u, editor.bsonObj().size());
    EXPECT_EQ(1, editor.bsonObj()[0].getIntField("a"));
    EXPECT_EQ(2, editor.bsonObj()[1].getIntField("b"));
    EXPECT_EQ(3, editor.bsonObj()[2].getIntField("c"));
}

TEST_F(DocumentTextEditorTests, prettyPrintedDocumentParses)
{
    DocumentTextEditor editor(users,
        "{\n    \"_id\" : 1,\n    \"tags\" : [\n        \"a\",\n        \"b\"\n    ],\n"
        "    \"at\" : ISODate(\"2023-11-14T22:13:20.123Z\")\n}");

    ASSERT_TRUE(editor.validate());

    ASSERT_EQ(1u, editor.bsonObj().size());
    EXPECT_EQ(1700000000123LL, editor.bsonObj()[0].getField("at").date().toMillisSinceEpoch());
}

TEST_F(DocumentTextEditorTests, validateReplacesPreviousResult)
{
    DocumentTextEditor editor(users, "{ a: 1 }\n{ b: 2 }");
    ASSERT_TRUE(editor.validate());

    ASSERT_TRUE(editor.validate());

    EXPECT_EQ(2u, editor.bsonObj().size());
}

TEST_F(DocumentTextEditorTests, readOnlyEditorStillParses)
{
    DocumentTextEditor editor(users, "{ a: 1 }", true);

    EXPECT_TRUE(editor.validate());
    EXPECT_EQ(1u, editor.bsonObj().size());
}
