#include <gtest/gtest.h>

#include "robomongo/core/domain/MongoNamespace.h"
#include "robomongo/core/domain/ResultsStaleness.h"

using namespace Robomongo;

namespace
{
    MongoNamespace const users("shop", "users");
    MongoNamespace const orders("shop", "orders");
    MongoNamespace const otherDbUsers("archive", "users");
}

TEST(ResultsStalenessTests, findResultsOutdatedAfterWriteToSameCollection)
{
    EXPECT_TRUE(ResultsStaleness::findResultsOutdated(false, users, users));
}

TEST(ResultsStalenessTests, findResultsCurrentWhenAutoRefreshReRunsQuery)
{
    EXPECT_FALSE(ResultsStaleness::findResultsOutdated(true, users, users));
}

TEST(ResultsStalenessTests, findResultsUnaffectedByOtherCollection)
{
    EXPECT_FALSE(ResultsStaleness::findResultsOutdated(false, users, orders));
}

TEST(ResultsStalenessTests, findResultsUnaffectedBySameCollectionNameInOtherDatabase)
{
    EXPECT_FALSE(ResultsStaleness::findResultsOutdated(false, users, otherDbUsers));
}

TEST(ResultsStalenessTests, findResultsUnaffectedByUnknownNamespace)
{
    EXPECT_FALSE(ResultsStaleness::findResultsOutdated(false, users, MongoNamespace()));
    EXPECT_FALSE(ResultsStaleness::findResultsOutdated(false, MongoNamespace(), MongoNamespace()));
}

TEST(ResultsStalenessTests, aggregateResultsOutdatedAfterWriteToSameCollection)
{
    EXPECT_TRUE(ResultsStaleness::aggregateResultsOutdated(false, "users", users));
}

TEST(ResultsStalenessTests, aggregateResultsCurrentWhenAutoRefreshReRunsQuery)
{
    EXPECT_FALSE(ResultsStaleness::aggregateResultsOutdated(true, "users", users));
}

TEST(ResultsStalenessTests, aggregateResultsUnaffectedByOtherCollection)
{
    EXPECT_FALSE(ResultsStaleness::aggregateResultsOutdated(false, "users", orders));
}

TEST(ResultsStalenessTests, aggregateResultsUnaffectedByUnknownNamespace)
{
    EXPECT_FALSE(ResultsStaleness::aggregateResultsOutdated(false, "users", MongoNamespace()));
    EXPECT_FALSE(ResultsStaleness::aggregateResultsOutdated(false, "", MongoNamespace()));
}
