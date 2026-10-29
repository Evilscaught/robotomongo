#include <gtest/gtest.h>

#include "robomongo/core/domain/CollectionOrder.h"

using namespace Robomongo;

TEST(CollectionOrderTests, sortsAlphabetically)
{
    std::vector<std::string> names { "orders", "users", "carts", "products" };

    CollectionOrder::sortNames(names);

    EXPECT_EQ(std::vector<std::string>({ "carts", "orders", "products", "users" }), names);
}

TEST(CollectionOrderTests, keepsOriginalCaseSensitiveOrder)
{
    // Byte-wise order, as Robo 3T has always listed collections: uppercase first
    std::vector<std::string> names { "apple", "Zebra", "banana", "Apple" };

    CollectionOrder::sortNames(names);

    EXPECT_EQ(std::vector<std::string>({ "Apple", "Zebra", "apple", "banana" }), names);
}

TEST(CollectionOrderTests, ordersByPrefixBeforeLongerNames)
{
    std::vector<std::string> names { "logs.2024", "logs", "logs_archive" };

    CollectionOrder::sortNames(names);

    EXPECT_EQ(std::vector<std::string>({ "logs", "logs.2024", "logs_archive" }), names);
}

TEST(CollectionOrderTests, sortsFullNamespacesTheSameWay)
{
    // The legacy path sorts "db.collection" strings; same database prefix, same order
    std::vector<std::string> names { "shop.users", "shop.Orders", "shop.carts" };

    CollectionOrder::sortNames(names);

    EXPECT_EQ(std::vector<std::string>({ "shop.Orders", "shop.carts", "shop.users" }), names);
}

TEST(CollectionOrderTests, handlesEmptyAndSingleLists)
{
    std::vector<std::string> empty;
    std::vector<std::string> single { "users" };

    CollectionOrder::sortNames(empty);
    CollectionOrder::sortNames(single);

    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(std::vector<std::string>({ "users" }), single);
}
