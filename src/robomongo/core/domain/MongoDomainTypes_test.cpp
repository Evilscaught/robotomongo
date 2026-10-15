#include <gtest/gtest.h>

#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo/core/domain/CursorPosition.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/domain/MongoStorageStats.h"

using namespace Robomongo;

TEST(CursorPositionTests, defaultIsNull)
{
    CursorPosition const position;

    EXPECT_TRUE(position.isNull());
    EXPECT_EQ(-1, position.line());
    EXPECT_EQ(-1, position.column());
}

TEST(CursorPositionTests, keepsLineAndColumn)
{
    CursorPosition const position(3, 7);

    EXPECT_FALSE(position.isNull());
    EXPECT_EQ(3, position.line());
    EXPECT_EQ(7, position.column());
}

TEST(CursorPositionTests, zeroAndNegativeAreNotNull)
{
    // Robo 3T uses CursorPosition(0, -2) to place the cursor inside "find({})"
    CursorPosition const position(0, -2);

    EXPECT_FALSE(position.isNull());
    EXPECT_EQ(0, position.line());
    EXPECT_EQ(-2, position.column());
}

TEST(MongoCollectionInfoTests, exposesNamespaceParts)
{
    MongoCollectionInfo const info("shop.users");

    EXPECT_EQ("users", info.name());
    EXPECT_EQ("shop.users", info.fullName());
    EXPECT_EQ("shop", info.ns().databaseName());
}

TEST(MongoCollectionInfoTests, keepsDottedCollectionNames)
{
    MongoCollectionInfo const info("media.fs.files");

    EXPECT_EQ("fs.files", info.name());
    EXPECT_EQ("media.fs.files", info.fullName());
}

TEST(MongoStorageStatsTests, collectionStatsKeepNameAndSize)
{
    MongoCollectionStorageStats const stats("users", 8192);

    EXPECT_EQ("users", stats.name);
    EXPECT_DOUBLE_EQ(8192, stats.storageSizeBytes);
    EXPECT_TRUE(stats.indexSizes.empty());
}

TEST(MongoStorageStatsTests, defaultsAreEmpty)
{
    MongoCollectionStorageStats const collection;
    MongoDatabaseStorageStats const database;

    EXPECT_EQ("", collection.name);
    EXPECT_DOUBLE_EQ(0, collection.storageSizeBytes);
    EXPECT_EQ("", database.name);
    EXPECT_DOUBLE_EQ(0, database.storageSizeBytes);
    EXPECT_TRUE(database.collections.empty());
}

TEST(MongoStorageStatsTests, databaseStatsKeepName)
{
    MongoDatabaseStorageStats const stats("shop");

    EXPECT_EQ("shop", stats.name);
    EXPECT_DOUBLE_EQ(0, stats.storageSizeBytes);
}

TEST(MongoFunctionTests, readsSystemJsDocument)
{
    mongo::BSONObjBuilder builder;
    builder.append("_id", "sum");
    builder.appendCode("value", "function (a, b) { return a + b; }");

    MongoFunction const function(builder.obj());

    EXPECT_EQ("sum", function.name());
    EXPECT_EQ("function (a, b) { return a + b; }", function.code());
}

TEST(MongoFunctionTests, writesSystemJsDocument)
{
    MongoFunction function;
    function.setName("sum");
    function.setCode("function (a, b) { return a + b; }");

    mongo::BSONObj const obj = function.toBson();

    EXPECT_EQ("sum", std::string(obj.getStringField("_id")));
    EXPECT_EQ(mongo::Code, obj.getField("value").type());
    EXPECT_EQ("function (a, b) { return a + b; }", obj.getField("value")._asCode());
}

TEST(MongoFunctionTests, roundTripsThroughBson)
{
    MongoFunction original;
    original.setName("greet");
    original.setCode("function (name) {\n    return 'hi ' + name;\n}");

    MongoFunction const copy(original.toBson());

    EXPECT_EQ(original.name(), copy.name());
    EXPECT_EQ(original.code(), copy.code());
}

TEST(MongoDocumentTests, defaultIsEmptyDocument)
{
    MongoDocument const document;

    EXPECT_TRUE(document.bsonObj().isEmpty());
}

TEST(MongoDocumentTests, keepsDocumentContent)
{
    mongo::BSONObj const obj = BSON("_id" << 1 << "name" << "Ada");

    MongoDocumentPtr const document = MongoDocument::fromBsonObj(obj);

    EXPECT_EQ(obj.toString(), document->bsonObj().toString());
}

TEST(MongoDocumentTests, convertsListsInOrder)
{
    std::vector<mongo::BSONObj> const objs { BSON("_id" << 1), BSON("_id" << 2), BSON("_id" << 3) };

    std::vector<MongoDocumentPtr> const documents = MongoDocument::fromBsonObj(objs);

    ASSERT_EQ(3u, documents.size());
    for (size_t i = 0; i < objs.size(); ++i)
        EXPECT_EQ(objs[i].toString(), documents[i]->bsonObj().toString());
}

TEST(MongoDocumentTests, convertsEmptyList)
{
    EXPECT_TRUE(MongoDocument::fromBsonObj(std::vector<mongo::BSONObj>()).empty());
}
