// Integration tests: run against a throwaway mongod (see TestMongoServer.h)

#include <gtest/gtest.h>

#include <algorithm>

#include <bsoncxx/builder/basic/document.hpp>
#include <mongo/bson/bsonobjbuilder.h>
#include <mongocxx/client.hpp>

#include "robomongo-unit-tests/TestMongoServer.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/events/MongoEventsInfo.h"
#include "robomongo/core/mongodb/modern/ModernMongoDriver.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/shell/bson/json.h"

using namespace Robomongo;

namespace
{
    TestMongoServer *server = nullptr;

    std::unique_ptr<ConnectionSettings> settingsFor(const TestMongoServer &mongod)
    {
        std::unique_ptr<ConnectionSettings> settings(new ConnectionSettings(false));
        settings->setServerHost("127.0.0.1");
        settings->setServerPort(mongod.port());
        return settings;
    }

    bool contains(const std::vector<std::string> &names, const std::string &name)
    {
        return std::find(names.begin(), names.end(), name) != names.end();
    }

    MongoQueryInfo findAll(const MongoNamespace &ns, int limit = 0, int skip = 0)
    {
        return MongoQueryInfo(CollectionInfo("127.0.0.1", ns.databaseName(), ns.collectionName()),
                              mongo::BSONObj(), mongo::BSONObj(), limit, skip, 0, 0, false);
    }

    std::vector<std::string> collectionNames(const ModernMongoDriver &driver, const std::string &db)
    {
        std::vector<std::string> names;
        for (auto const& info : driver.listCollections(db))
            names.push_back(info.name());
        return names;
    }
}

class ModernMongoDriverTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { server = new TestMongoServer(); }
    static void TearDownTestCase() { delete server; server = nullptr; }

    void SetUp() override
    {
        if (server && server->available()) {
            settings = settingsFor(*server);
            driver.reset(new ModernMongoDriver(*settings));
        }
    }

    // A database name unique to the running test
    std::string db() const
    {
        return std::string("it_") + ::testing::UnitTest::GetInstance()->current_test_info()->name();
    }

    std::unique_ptr<ConnectionSettings> settings;
    std::unique_ptr<ModernMongoDriver> driver;
};

TEST_F(ModernMongoDriverTests, listsDatabases)
{
    ROBO_REQUIRE_MONGOD(server);
    driver->insertDocument(MongoNamespace(db(), "c"), BSON("_id" << 1));

    std::vector<std::string> const names = driver->listDatabaseNames();

    EXPECT_TRUE(contains(names, "admin"));
    EXPECT_TRUE(contains(names, db()));
}

TEST_F(ModernMongoDriverTests, listsCollectionsSorted)
{
    ROBO_REQUIRE_MONGOD(server);
    for (const char *name : { "zebra", "Apple", "mango", "apple" })
        driver->createCollection(MongoNamespace(db(), name));

    std::vector<MongoCollectionInfo> const collections = driver->listCollections(db());

    std::vector<std::string> names;
    for (auto const& info : collections) {
        names.push_back(info.name());
        EXPECT_EQ(db() + "." + info.name(), info.fullName());
    }
    EXPECT_EQ(std::vector<std::string>({ "Apple", "apple", "mango", "zebra" }), names);
}

TEST_F(ModernMongoDriverTests, insertsAndFindsDocuments)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "users");
    driver->insertDocument(ns, BSON("_id" << 1 << "name" << "Ada" << "tags" << BSON_ARRAY("x" << "y")));
    driver->insertDocument(ns, BSON("_id" << 2 << "name" << "Bob"));

    std::vector<MongoDocumentPtr> const docs = driver->find(findAll(ns));

    ASSERT_EQ(2u, docs.size());
    EXPECT_EQ(std::string("Ada"), docs[0]->bsonObj().getStringField("name"));
    EXPECT_EQ(2, docs[0]->bsonObj().getField("tags").Obj().nFields());
    EXPECT_EQ(std::string("Bob"), docs[1]->bsonObj().getStringField("name"));
}

TEST_F(ModernMongoDriverTests, findAppliesFilterSkipAndLimit)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "numbers");
    for (int i = 0; i < 10; ++i)
        driver->insertDocument(ns, BSON("_id" << i << "even" << (i % 2 == 0) << "secret" << "x"));

    MongoQueryInfo query(CollectionInfo("127.0.0.1", ns.databaseName(), ns.collectionName()),
                         BSON("even" << true), mongo::BSONObj(), 2, 1, 0, 0, false);
    std::vector<MongoDocumentPtr> const docs = driver->find(query);

    ASSERT_EQ(2u, docs.size());
    EXPECT_EQ(2, docs[0]->bsonObj().getIntField("_id"));
    EXPECT_EQ(4, docs[1]->bsonObj().getIntField("_id"));
}

TEST_F(ModernMongoDriverTests, saveReplacesOrInsertsById)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "docs");
    driver->insertDocument(ns, BSON("_id" << 1 << "v" << "old" << "gone" << true));

    driver->saveDocument(ns, BSON("_id" << 1 << "v" << "new"));
    driver->saveDocument(ns, BSON("_id" << 2 << "v" << "upserted"));

    std::vector<MongoDocumentPtr> const docs = driver->find(findAll(ns));
    ASSERT_EQ(2u, docs.size());
    EXPECT_EQ(std::string("new"), docs[0]->bsonObj().getStringField("v"));
    EXPECT_FALSE(docs[0]->bsonObj().hasField("gone"));
    EXPECT_EQ(std::string("upserted"), docs[1]->bsonObj().getStringField("v"));
}

TEST_F(ModernMongoDriverTests, saveRequiresId)
{
    ROBO_REQUIRE_MONGOD(server);

    EXPECT_THROW(driver->saveDocument(MongoNamespace(db(), "docs"), BSON("v" << 1)), std::runtime_error);
}

TEST_F(ModernMongoDriverTests, removesOneOrManyMatchingDocuments)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "items");
    for (int i = 0; i < 5; ++i)
        driver->insertDocument(ns, BSON("_id" << i << "kind" << (i < 3 ? "a" : "b")));

    driver->removeDocuments(ns, mongo::Query(BSON("kind" << "a")), true);
    EXPECT_EQ(4u, driver->find(findAll(ns)).size());

    driver->removeDocuments(ns, mongo::Query(BSON("kind" << "a")), false);
    std::vector<MongoDocumentPtr> const left = driver->find(findAll(ns));
    ASSERT_EQ(2u, left.size());
    EXPECT_EQ(std::string("b"), left[0]->bsonObj().getStringField("kind"));
}

TEST_F(ModernMongoDriverTests, duplicateKeyInsertFails)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "unique");
    driver->insertDocument(ns, BSON("_id" << 1));

    EXPECT_ANY_THROW(driver->insertDocument(ns, BSON("_id" << 1)));
}

TEST_F(ModernMongoDriverTests, createsRenamesAndDropsCollections)
{
    ROBO_REQUIRE_MONGOD(server);
    driver->createCollection(MongoNamespace(db(), "old"));

    driver->renameCollection(MongoNamespace(db(), "old"), "renamed");
    EXPECT_EQ(std::vector<std::string>({ "renamed" }), collectionNames(*driver, db()));

    driver->dropCollection(MongoNamespace(db(), "renamed"));
    EXPECT_TRUE(collectionNames(*driver, db()).empty());
}

TEST_F(ModernMongoDriverTests, duplicateCollectionCopiesDocuments)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const source(db(), "source");
    for (int i = 0; i < 1200; ++i)   // more than one insert batch (500)
        driver->insertDocument(source, BSON("_id" << i));

    driver->duplicateCollection(source, "copy");

    EXPECT_EQ(1200u, driver->find(findAll(MongoNamespace(db(), "copy"))).size());
    EXPECT_EQ(1200u, driver->find(findAll(source)).size());
}

TEST_F(ModernMongoDriverTests, copyRefusesExistingTarget)
{
    ROBO_REQUIRE_MONGOD(server);
    driver->insertDocument(MongoNamespace(db(), "a"), BSON("_id" << 1));
    driver->createCollection(MongoNamespace(db(), "b"));

    EXPECT_THROW(driver->copyCollectionTo(MongoNamespace(db(), "a"), MongoNamespace(db(), "b")),
                 std::runtime_error);
}

TEST_F(ModernMongoDriverTests, dropsDatabase)
{
    ROBO_REQUIRE_MONGOD(server);
    driver->insertDocument(MongoNamespace(db(), "c"), BSON("_id" << 1));

    driver->dropDatabase(db());

    EXPECT_FALSE(contains(driver->listDatabaseNames(), db()));
}

TEST_F(ModernMongoDriverTests, createsAndListsIndexes)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "people");
    driver->createCollection(ns);
    MongoCollectionInfo const collection(ns.toString());
    IndexInfo const index(collection, "email_1", "{ \"email\" : 1 }", true, false, true);

    driver->addEditIndex(IndexInfo(collection), index);

    std::vector<IndexInfo> const indexes = driver->listIndexes(collection);
    ASSERT_EQ(2u, indexes.size());   // _id_ and email_1
    auto const email = std::find_if(indexes.begin(), indexes.end(),
                                     [](const IndexInfo &i) { return i._name == "email_1"; });
    ASSERT_NE(indexes.end(), email);
    EXPECT_TRUE(email->_unique);
    EXPECT_TRUE(email->_sparse);
    EXPECT_EQ(1, mongo::Robomongo::fromjson(email->_keys).getIntField("email"));
}

TEST_F(ModernMongoDriverTests, uniqueIndexIsEnforced)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "people");
    MongoCollectionInfo const collection(ns.toString());
    driver->createCollection(ns);
    driver->addEditIndex(IndexInfo(collection), IndexInfo(collection, "email_1", "{ \"email\" : 1 }", true));
    driver->insertDocument(ns, BSON("email" << "a@example.com"));

    EXPECT_ANY_THROW(driver->insertDocument(ns, BSON("email" << "a@example.com")));
}

TEST_F(ModernMongoDriverTests, createsTtlIndex)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "events");
    MongoCollectionInfo const collection(ns.toString());
    driver->createCollection(ns);

    driver->addEditIndex(IndexInfo(collection),
                         IndexInfo(collection, "at_1", "{ \"at\" : 1 }", false, false, false, 3600));

    std::vector<IndexInfo> const indexes = driver->listIndexes(collection);
    auto const ttl = std::find_if(indexes.begin(), indexes.end(),
                                  [](const IndexInfo &i) { return i._name == "at_1"; });
    ASSERT_NE(indexes.end(), ttl);
    EXPECT_EQ(3600, ttl->_ttl);
}

TEST_F(ModernMongoDriverTests, createsTextIndexWithLanguage)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "articles");
    MongoCollectionInfo const collection(ns.toString());
    driver->createCollection(ns);

    driver->addEditIndex(IndexInfo(collection),
                         IndexInfo(collection, "body_text", "{ \"body\" : \"text\" }", false, false, false, -1,
                                   "spanish", "idioma"));

    std::vector<IndexInfo> const indexes = driver->listIndexes(collection);
    auto const text = std::find_if(indexes.begin(), indexes.end(),
                                   [](const IndexInfo &i) { return i._name == "body_text"; });
    ASSERT_NE(indexes.end(), text);
    EXPECT_EQ("spanish", text->_defaultLanguage);
    EXPECT_EQ("idioma", text->_languageOverride);
}

TEST_F(ModernMongoDriverTests, editingIndexReplacesIt)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "people");
    MongoCollectionInfo const collection(ns.toString());
    driver->createCollection(ns);
    IndexInfo const original(collection, "name_1", "{ \"name\" : 1 }");
    driver->addEditIndex(IndexInfo(collection), original);

    driver->addEditIndex(original, IndexInfo(collection, "name_desc", "{ \"name\" : -1 }"));

    std::vector<std::string> names;
    for (auto const& index : driver->listIndexes(collection))
        names.push_back(index._name);
    EXPECT_TRUE(contains(names, "name_desc"));
    EXPECT_FALSE(contains(names, "name_1"));
}

TEST_F(ModernMongoDriverTests, failedIndexEditRestoresOriginal)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "people");
    MongoCollectionInfo const collection(ns.toString());
    driver->insertDocument(ns, BSON("email" << "dup@example.com"));
    driver->insertDocument(ns, BSON("email" << "dup@example.com"));
    IndexInfo const original(collection, "email_1", "{ \"email\" : 1 }");
    driver->addEditIndex(IndexInfo(collection), original);

    // Making it unique fails because of the duplicate values
    EXPECT_ANY_THROW(driver->addEditIndex(original, IndexInfo(collection, "email_1", "{ \"email\" : 1 }", true)));

    std::vector<std::string> names;
    for (auto const& index : driver->listIndexes(collection))
        names.push_back(index._name);
    EXPECT_TRUE(contains(names, "email_1"));
}

TEST_F(ModernMongoDriverTests, dropsIndex)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "people");
    MongoCollectionInfo const collection(ns.toString());
    driver->createCollection(ns);
    driver->addEditIndex(IndexInfo(collection), IndexInfo(collection, "name_1", "{ \"name\" : 1 }"));

    driver->dropIndex(collection, "name_1");

    EXPECT_EQ(1u, driver->listIndexes(collection).size());
}

TEST_F(ModernMongoDriverTests, loadsStorageStats)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "sized");
    MongoCollectionInfo const collection(ns.toString());
    for (int i = 0; i < 100; ++i)
        driver->insertDocument(ns, BSON("_id" << i << "payload" << std::string(1000, 'x')));
    driver->addEditIndex(IndexInfo(collection), IndexInfo(collection, "payload_1", "{ \"payload\" : 1 }"));

    MongoDatabaseStorageStats const stats = driver->loadStorageStats(db());

    EXPECT_EQ(db(), stats.name);
    EXPECT_GT(stats.storageSizeBytes, 0);
    ASSERT_EQ(1u, stats.collections.size());
    EXPECT_EQ("sized", stats.collections[0].name);
    EXPECT_GT(stats.collections[0].storageSizeBytes, 0);
    EXPECT_EQ(2u, stats.collections[0].indexSizes.size());
}
