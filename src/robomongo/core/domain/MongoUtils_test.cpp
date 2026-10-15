#include <gtest/gtest.h>

#include "robomongo/core/domain/MongoUtils.h"

using namespace Robomongo;

TEST(MongoUtilsTests, smallSizesAreShownInKilobytes)
{
    EXPECT_EQ(QString("0.00 kb"), MongoUtils::buildNiceSizeString(0));
    EXPECT_EQ(QString("1.00 kb"), MongoUtils::buildNiceSizeString(1024));
    EXPECT_EQ(QString("1.50 kb"), MongoUtils::buildNiceSizeString(1536));
}

TEST(MongoUtilsTests, switchesToMegabytesAt100Kilobytes)
{
    EXPECT_EQ(QString("99.99 kb"), MongoUtils::buildNiceSizeString(1024 * 100 - 10));
    EXPECT_EQ(QString("0.10 mb"), MongoUtils::buildNiceSizeString(1024 * 100));
}

TEST(MongoUtilsTests, largeSizesAreShownInMegabytes)
{
    EXPECT_EQ(QString("1.00 mb"), MongoUtils::buildNiceSizeString(1024 * 1024));
    EXPECT_EQ(QString("1024.00 mb"), MongoUtils::buildNiceSizeString(1024.0 * 1024 * 1024));
}

TEST(MongoUtilsTests, passwordHashIsMongodbCrDigest)
{
    // md5("<user>:mongo:<password>") in hex, as used by MONGODB-CR authentication
    EXPECT_EQ("5cb0202c5c5061015a8b227696b4bfe3", MongoUtils::buildPasswordHash("admin", "secret"));
}

TEST(MongoUtilsTests, passwordHashDependsOnUserAndPassword)
{
    std::string const base = MongoUtils::buildPasswordHash("admin", "secret");

    EXPECT_NE(base, MongoUtils::buildPasswordHash("Admin", "secret"));
    EXPECT_NE(base, MongoUtils::buildPasswordHash("admin", "Secret"));
    EXPECT_EQ(32u, base.size());
}
