#include <gtest/gtest.h>

#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo/core/domain/MongoQueryInfo.h"

using namespace Robomongo;

TEST(MongoQueryInfoTests, prepareServerAddressKeepsPlainAddress)
{
    EXPECT_EQ("localhost:27017", detail::prepareServerAddress("localhost:27017"));
}

TEST(MongoQueryInfoTests, prepareServerAddressDropsReplicaSetSuffix)
{
    // The shell reports replica set hosts as "address{...}"
    EXPECT_EQ("rs0/a:27017,b:27017",
              detail::prepareServerAddress("rs0/a:27017,b:27017{ ok: 1 }"));
}

TEST(MongoQueryInfoTests, prepareServerAddressCutsAtFirstBrace)
{
    EXPECT_EQ("host", detail::prepareServerAddress("host{a}{b}"));
    EXPECT_EQ("", detail::prepareServerAddress("{only}"));
    EXPECT_EQ("", detail::prepareServerAddress(""));
}

TEST(MongoQueryInfoTests, defaultCollectionInfoIsInvalid)
{
    EXPECT_FALSE(CollectionInfo().isValid());
}

TEST(MongoQueryInfoTests, collectionInfoNeedsServerAddress)
{
    EXPECT_FALSE(CollectionInfo("", "shop", "users").isValid());
    EXPECT_TRUE(CollectionInfo("localhost:27017", "shop", "users").isValid());
}

TEST(MongoQueryInfoTests, collectionInfoBuildsNamespace)
{
    CollectionInfo const info("localhost:27017", "shop", "users");

    EXPECT_EQ("localhost:27017", info._serverAddress);
    EXPECT_EQ("shop.users", info._ns.toString());
}

TEST(MongoQueryInfoTests, defaultQueryInfoHasInvalidCollection)
{
    MongoQueryInfo const info;

    EXPECT_FALSE(info._info.isValid());
    EXPECT_TRUE(info._query.isEmpty());
    EXPECT_TRUE(info._fields.isEmpty());
}

TEST(MongoQueryInfoTests, keepsAllQueryParameters)
{
    mongo::BSONObj const query = BSON("age" << BSON("$gt" << 21));
    mongo::BSONObj const fields = BSON("name" << 1);

    MongoQueryInfo const info(CollectionInfo("localhost:27017", "shop", "users"),
                              query, fields, 50, 10, 25, 4, true);

    EXPECT_EQ("shop.users", info._info._ns.toString());
    EXPECT_EQ(query.toString(), info._query.toString());
    EXPECT_EQ(fields.toString(), info._fields.toString());
    EXPECT_EQ(50, info._limit);
    EXPECT_EQ(10, info._skip);
    EXPECT_EQ(25, info._batchSize);
    EXPECT_EQ(4, info._options);
    EXPECT_TRUE(info._special);
}
