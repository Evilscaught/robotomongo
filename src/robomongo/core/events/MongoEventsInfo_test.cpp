#include <gtest/gtest.h>

#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo/core/events/MongoEventsInfo.h"
#include "robomongo/shell/bson/json.h"

using namespace Robomongo;

namespace
{
    MongoCollectionInfo const users("shop.users");

    IndexInfo parse(const std::string &json)
    {
        return makeIndexInfoFromBsonObj(users, mongo::Robomongo::fromjson(json));
    }
}

TEST(IndexInfoTests, defaults)
{
    IndexInfo const info(users);

    EXPECT_EQ("shop.users", info._collection.fullName());
    EXPECT_EQ("", info._name);
    EXPECT_EQ("", info._keys);
    EXPECT_FALSE(info._unique);
    EXPECT_FALSE(info._backGround);
    EXPECT_FALSE(info._sparse);
    EXPECT_EQ(-1, info._ttl);
    EXPECT_EQ("", info._textWeights);
}

TEST(IndexInfoTests, parsesSimpleIndex)
{
    IndexInfo const info = parse(R"({ v: 2, key: { email: 1 }, name: "email_1" })");

    EXPECT_EQ("email_1", info._name);
    EXPECT_EQ("shop.users", info._collection.fullName());
    EXPECT_EQ(1, mongo::Robomongo::fromjson(info._keys).getField("email").numberInt());
    EXPECT_FALSE(info._unique);
    EXPECT_FALSE(info._sparse);
    EXPECT_FALSE(info._backGround);
}

TEST(IndexInfoTests, keepsCompoundKeyOrderAndDirections)
{
    IndexInfo const info = parse(R"({ key: { lastName: 1, firstName: -1, age: 1 }, name: "compound" })");

    mongo::BSONObj const keys = mongo::Robomongo::fromjson(info._keys);
    EXPECT_EQ("{ lastName: 1, firstName: -1, age: 1 }", keys.toString());
}

TEST(IndexInfoTests, keepsSpecialIndexTypes)
{
    IndexInfo const geo = parse(R"({ key: { location: "2dsphere" }, name: "location_2dsphere" })");
    IndexInfo const hashed = parse(R"({ key: { _id: "hashed" }, name: "_id_hashed" })");

    EXPECT_EQ("2dsphere", mongo::Robomongo::fromjson(geo._keys).getField("location").str());
    EXPECT_EQ("hashed", mongo::Robomongo::fromjson(hashed._keys).getField("_id").str());
}

TEST(IndexInfoTests, parsesOptions)
{
    IndexInfo const info = parse(
        R"({ key: { email: 1 }, name: "email_1", unique: true, sparse: true, background: true })");

    EXPECT_TRUE(info._unique);
    EXPECT_TRUE(info._sparse);
    EXPECT_TRUE(info._backGround);
}

TEST(IndexInfoTests, parsesTtlInAnyNumericType)
{
    EXPECT_EQ(3600, parse(R"({ key: { at: 1 }, name: "a", expireAfterSeconds: 3600 })")._ttl);
    EXPECT_EQ(3600, parse(R"({ key: { at: 1 }, name: "b", expireAfterSeconds: NumberLong(3600) })")._ttl);
    EXPECT_EQ(3600, parse(R"({ key: { at: 1 }, name: "c", expireAfterSeconds: 3600.0 })")._ttl);
    EXPECT_EQ(0, parse(R"({ key: { at: 1 }, name: "d", expireAfterSeconds: 0 })")._ttl);
}

TEST(IndexInfoTests, parsesTextIndexLanguageAndWeights)
{
    IndexInfo const info = parse(
        R"({ key: { _fts: "text", _ftsx: 1 }, name: "content_text", weights: { title: 10, body: 1 },
             default_language: "spanish", language_override: "idioma", textIndexVersion: 3 })");

    EXPECT_EQ("spanish", info._defaultLanguage);
    EXPECT_EQ("idioma", info._languageOverride);
    mongo::BSONObj const weights = mongo::Robomongo::fromjson(info._textWeights);
    EXPECT_EQ(10, weights.getField("title").numberInt());
    EXPECT_EQ(1, weights.getField("body").numberInt());
}

TEST(IndexInfoTests, keysAreShownAsEditableJson)
{
    IndexInfo const info = parse(R"({ key: { email: 1 }, name: "email_1" })");

    EXPECT_EQ("{\n    \"email\" : 1\n}", info._keys);
}

TEST(IndexInfoTests, constructorKeepsAllFields)
{
    IndexInfo const info(users, "name_1", "{ name: 1 }", true, true, true, 60, "english", "lang", "{ name: 5 }");

    EXPECT_EQ("name_1", info._name);
    EXPECT_EQ("{ name: 1 }", info._keys);
    EXPECT_TRUE(info._unique);
    EXPECT_TRUE(info._backGround);
    EXPECT_TRUE(info._sparse);
    EXPECT_EQ(60, info._ttl);
    EXPECT_EQ("english", info._defaultLanguage);
    EXPECT_EQ("lang", info._languageOverride);
    EXPECT_EQ("{ name: 5 }", info._textWeights);
}

TEST(ConnectionInfoTests, defaultsToLegacyDriver)
{
    ConnectionInfo const info("{uuid}");

    EXPECT_EQ("Legacy driver", info._driverName);
    EXPECT_EQ("4.2", info._driverVersion);
    EXPECT_EQ(0, info._maxWireVersion);
    EXPECT_EQ("{uuid}", info._uuid);
    EXPECT_TRUE(info._databases.empty());
}

TEST(ConnectionInfoTests, keepsServerDetails)
{
    ConnectionInfo const info("db:27017", { "admin", "shop" }, 7.0f, "7.0.4", "wiredTiger", "{uuid}",
                              "Modern driver", "4.6.1", 21);

    EXPECT_EQ("db:27017", info._address);
    EXPECT_EQ(std::vector<std::string>({ "admin", "shop" }), info._databases);
    EXPECT_EQ("7.0.4", info._dbVersionStr);
    EXPECT_EQ("wiredTiger", info._storageEngineType);
    EXPECT_EQ("Modern driver", info._driverName);
    EXPECT_EQ("4.6.1", info._driverVersion);
    EXPECT_EQ(21, info._maxWireVersion);
}

TEST(IndexInfoTests, indexWithoutTtlHasNoTtl)
{
    // The index dialog shows "Expire after" checked when _ttl >= 0
    IndexInfo const info = parse(R"({ key: { email: 1 }, name: "email_1" })");

    EXPECT_LT(info._ttl, 0);
}
