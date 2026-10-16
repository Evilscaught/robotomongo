#include <gtest/gtest.h>

#include "robomongo/core/domain/MongoNamespace.h"

using namespace Robomongo;

TEST(MongoNamespaceTests, defaultIsInvalidAndEmpty)
{
    MongoNamespace const ns;

    EXPECT_FALSE(ns.isValid());
    EXPECT_EQ("", ns.toString());
    EXPECT_EQ("", ns.databaseName());
    EXPECT_EQ("", ns.collectionName());
}

TEST(MongoNamespaceTests, parsesFullNamespace)
{
    MongoNamespace const ns("shop.users");

    EXPECT_TRUE(ns.isValid());
    EXPECT_EQ("shop.users", ns.toString());
    EXPECT_EQ("shop", ns.databaseName());
    EXPECT_EQ("users", ns.collectionName());
}

TEST(MongoNamespaceTests, splitsOnFirstDotOnly)
{
    // Collection names may contain dots (e.g. system.users, GridFS fs.files)
    MongoNamespace const ns("admin.system.users");

    EXPECT_EQ("admin", ns.databaseName());
    EXPECT_EQ("system.users", ns.collectionName());
}

TEST(MongoNamespaceTests, keepsGridFsCollectionNames)
{
    MongoNamespace const ns("media.fs.chunks");

    EXPECT_EQ("media", ns.databaseName());
    EXPECT_EQ("fs.chunks", ns.collectionName());
}

TEST(MongoNamespaceTests, buildsFromDatabaseAndCollection)
{
    MongoNamespace const ns("shop", "users");

    EXPECT_TRUE(ns.isValid());
    EXPECT_EQ("shop.users", ns.toString());
    EXPECT_EQ("shop", ns.databaseName());
    EXPECT_EQ("users", ns.collectionName());
}

TEST(MongoNamespaceTests, bothConstructorsAgree)
{
    MongoNamespace const parsed("admin.system.version");
    MongoNamespace const built("admin", "system.version");

    EXPECT_EQ(built.toString(), parsed.toString());
    EXPECT_EQ(built.databaseName(), parsed.databaseName());
    EXPECT_EQ(built.collectionName(), parsed.collectionName());
}

TEST(MongoNamespaceTests, keepsUnicodeAndSpaces)
{
    MongoNamespace const ns("café", "order items");

    EXPECT_EQ("café.order items", ns.toString());
    MongoNamespace const parsed(ns.toString());
    EXPECT_EQ("café", parsed.databaseName());
    EXPECT_EQ("order items", parsed.collectionName());
}

TEST(MongoNamespaceTests, emptyCollectionNameStillHasDatabase)
{
    MongoNamespace const ns("shop.");

    EXPECT_EQ("shop", ns.databaseName());
    EXPECT_EQ("", ns.collectionName());
}
