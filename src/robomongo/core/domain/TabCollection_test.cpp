#include <gtest/gtest.h>

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/TabCollection.h"

using namespace Robomongo;

namespace
{
    MongoShellResult findResult(const std::string &database, const std::string &collection)
    {
        MongoQueryInfo const info(CollectionInfo("localhost:27017", database, collection),
                                  mongo::BSONObj(), mongo::BSONObj(), 0, 0, 0, 0, false);
        return MongoShellResult("find", "", {}, info, "find", 0);
    }

    MongoShellResult aggregateResult(const std::string &collection)
    {
        AggrInfo const aggrInfo(collection, 0, 50, mongo::BSONObj(), mongo::BSONObj(), 0);
        return MongoShellResult("aggregate", "", {}, MongoQueryInfo(), "aggregate", 0, aggrInfo);
    }

    MongoShellResult plainResult()
    {
        return MongoShellResult("", "{ \"ok\" : 1 }", {}, MongoQueryInfo(), "db.stats()", 0);
    }
}

TEST(TabCollectionTests, usesNamespaceOfFindResult)
{
    MongoNamespace const ns = TabCollection::fromResults({ findResult("shop", "users") }, "other");

    EXPECT_EQ("shop.users", ns.toString());
}

TEST(TabCollectionTests, skipsResultsThatAreNotCollections)
{
    MongoNamespace const ns = TabCollection::fromResults(
        { plainResult(), findResult("shop", "users") }, "shop");

    EXPECT_EQ("shop.users", ns.toString());
}

TEST(TabCollectionTests, usesFirstFindResult)
{
    MongoNamespace const ns = TabCollection::fromResults(
        { findResult("shop", "users"), findResult("shop", "orders") }, "shop");

    EXPECT_EQ("shop.users", ns.toString());
}

TEST(TabCollectionTests, prefersFindResultOverEarlierAggregate)
{
    MongoNamespace const ns = TabCollection::fromResults(
        { aggregateResult("orders"), findResult("shop", "users") }, "shop");

    EXPECT_EQ("shop.users", ns.toString());
}

TEST(TabCollectionTests, usesAggregateCollectionInCurrentDatabase)
{
    MongoNamespace const ns = TabCollection::fromResults(
        { plainResult(), aggregateResult("orders") }, "shop");

    EXPECT_EQ("shop.orders", ns.toString());
}

TEST(TabCollectionTests, aggregateNeedsCurrentDatabase)
{
    MongoNamespace const ns = TabCollection::fromResults({ aggregateResult("orders") }, "");

    EXPECT_FALSE(ns.isValid());
}

TEST(TabCollectionTests, aggregateNeedsCollectionName)
{
    MongoNamespace const ns = TabCollection::fromResults({ aggregateResult("") }, "shop");

    EXPECT_FALSE(ns.isValid());
}

TEST(TabCollectionTests, noCollectionWithoutCollectionResults)
{
    EXPECT_FALSE(TabCollection::fromResults({}, "shop").isValid());
    EXPECT_FALSE(TabCollection::fromResults({ plainResult() }, "shop").isValid());
}
