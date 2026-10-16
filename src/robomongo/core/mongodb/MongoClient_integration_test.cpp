// Integration tests for the legacy driver path (embedded MongoDB 4.2 client), against a throwaway
// mongod (see TestMongoServer.h)

#include <gtest/gtest.h>

#include <algorithm>

#include <mongo/bson/bsonobjbuilder.h>
#include <mongo/client/dbclient_connection.h>

#include "robomongo-unit-tests/LegacyMongoRuntime.h"
#include "robomongo-unit-tests/TestMongoServer.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/events/MongoEventsInfo.h"
#include "robomongo/core/mongodb/MongoClient.h"

using namespace Robomongo;

namespace
{
    TestMongoServer *server = nullptr;

    bool contains(const std::vector<std::string> &names, const std::string &name)
    {
        return std::find(names.begin(), names.end(), name) != names.end();
    }

    MongoQueryInfo findAll(const MongoNamespace &ns)
    {
        return MongoQueryInfo(CollectionInfo("127.0.0.1", ns.databaseName(), ns.collectionName()),
                              mongo::BSONObj(), mongo::BSONObj(), 0, 0, 0, 0, false);
    }
}

class LegacyMongoClientTests : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ensureLegacyMongoRuntime();
        server = new TestMongoServer();
    }
    static void TearDownTestCase() { delete server; server = nullptr; }

    void SetUp() override
    {
        if (!server || !server->available())
            return;
        connection.reset(new mongo::DBClientConnection(true, 10));
        ASSERT_TRUE(connection->connect(mongo::HostAndPort("127.0.0.1", server->port()), "robo-tests").isOK());
        client.reset(new MongoClient(connection.get()));
    }

    std::string db() const
    {
        return std::string("legacy_") + ::testing::UnitTest::GetInstance()->current_test_info()->name();
    }

    std::unique_ptr<mongo::DBClientConnection> connection;
    std::unique_ptr<MongoClient> client;
};

TEST_F(LegacyMongoClientTests, reportsServerVersionAndEngine)
{
    ROBO_REQUIRE_MONGOD(server);

    std::string const version = client->dbVersionStr();

    EXPECT_FALSE(version.empty());
    EXPECT_GE(client->getVersion(), 4.0f);
    EXPECT_EQ("wiredTiger", client->getStorageEngineType());
}

TEST_F(LegacyMongoClientTests, listsDatabasesAndSortedCollections)
{
    ROBO_REQUIRE_MONGOD(server);
    for (const char *name : { "zebra", "Apple", "mango" })
        client->insertDocument(BSON("_id" << 1), MongoNamespace(db(), name));

    std::vector<std::string> const collections = client->getCollectionNamesWithDbname(db());

    EXPECT_TRUE(contains(client->getDatabaseNames(), db()));
    EXPECT_EQ(std::vector<std::string>({ db() + ".Apple", db() + ".mango", db() + ".zebra" }), collections);
}

TEST_F(LegacyMongoClientTests, insertSaveQueryAndRemove)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "users");

    client->insertDocument(BSON("_id" << 1 << "name" << "Ada"), ns);
    client->insertDocument(BSON("_id" << 2 << "name" << "Bob"), ns);
    client->saveDocument(BSON("_id" << 1 << "name" << "Ada Lovelace"), ns);
    client->removeDocuments(ns, mongo::Query(BSON("_id" << 2)));

    std::vector<MongoDocumentPtr> const docs = client->query(findAll(ns));
    ASSERT_EQ(1u, docs.size());
    EXPECT_EQ(std::string("Ada Lovelace"), docs[0]->bsonObj().getStringField("name"));
}

TEST_F(LegacyMongoClientTests, keepsFieldTypes)
{
    // The legacy path sends BSON directly, so types survive (compare the modern driver path)
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "types");
    mongo::BSONObjBuilder builder;
    builder.append("_id", 1);
    builder.append("whole", 5.0);
    builder.append("small64", 5LL);
    builder.append("dec", mongo::Decimal128("1.10"));
    builder.appendCode("code", "function () {}");
    client->insertDocument(builder.obj(), ns);

    mongo::BSONObj const stored = client->query(findAll(ns))[0]->bsonObj();

    EXPECT_EQ(mongo::NumberDouble, stored.getField("whole").type());
    EXPECT_EQ(mongo::NumberLong, stored.getField("small64").type());
    EXPECT_EQ(mongo::NumberDecimal, stored.getField("dec").type());
    EXPECT_EQ(mongo::Code, stored.getField("code").type());
}

TEST_F(LegacyMongoClientTests, createsCappedCollectionWithOptions)
{
    ROBO_REQUIRE_MONGOD(server);
    std::string const ns = db() + ".capped";

    client->createCollection(ns, 1024 * 1024, true, 10, BSON("validator" << BSON("v" << BSON("$exists" << true))));

    mongo::BSONObj stats;
    ASSERT_TRUE(connection->runCommand(db(), BSON("collStats" << "capped"), stats));
    EXPECT_TRUE(stats.getBoolField("capped"));
    EXPECT_EQ(10, stats.getIntField("max"));
    mongo::BSONObj info;
    ASSERT_TRUE(connection->runCommand(db(), BSON("listCollections" << 1 << "filter" << BSON("name" << "capped")), info));
    EXPECT_NE(std::string::npos, info.toString().find("validator"));
}

TEST_F(LegacyMongoClientTests, renamesDuplicatesAndDropsCollections)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "orig");
    client->insertDocument(BSON("_id" << 1), ns);

    client->duplicateCollection(ns, "copy");
    client->renameCollection(ns, "renamed");
    client->dropCollection(MongoNamespace(db(), "copy"));

    std::vector<std::string> const collections = client->getCollectionNamesWithDbname(db());
    EXPECT_EQ(std::vector<std::string>({ db() + ".renamed" }), collections);
}

TEST_F(LegacyMongoClientTests, dropsDatabase)
{
    ROBO_REQUIRE_MONGOD(server);
    client->insertDocument(BSON("_id" << 1), MongoNamespace(db(), "c"));

    client->dropDatabase(db());

    EXPECT_FALSE(contains(client->getDatabaseNames(), db()));
}

TEST_F(LegacyMongoClientTests, managesIndexes)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoNamespace const ns(db(), "people");
    MongoCollectionInfo const collection(ns.toString());
    client->insertDocument(BSON("_id" << 1 << "email" << "a@example.com"), ns);

    client->addEditIndex(IndexInfo(collection),
                         IndexInfo(collection, "email_1", "{ \"email\" : 1 }", true, false, true, 3600));
    std::vector<IndexInfo> indexes = client->getIndexes(collection);
    auto const email = std::find_if(indexes.begin(), indexes.end(),
                                    [](const IndexInfo &i) { return i._name == "email_1"; });
    ASSERT_NE(indexes.end(), email);
    EXPECT_TRUE(email->_unique);
    EXPECT_TRUE(email->_sparse);
    EXPECT_EQ(3600, email->_ttl);

    client->dropIndexFromCollection(collection, "email_1");
    EXPECT_EQ(1u, client->getIndexes(collection).size());
}

TEST_F(LegacyMongoClientTests, managesStoredFunctions)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoFunction sum;
    sum.setName("sum");
    sum.setCode("function (a, b) { return a + b; }");

    client->createFunction(db(), sum);
    std::vector<MongoFunction> functions = client->getFunctions(db());
    ASSERT_EQ(1u, functions.size());
    EXPECT_EQ("sum", functions[0].name());

    MongoFunction renamed = sum;
    renamed.setName("add");
    client->createFunction(db(), renamed, "sum");
    functions = client->getFunctions(db());
    ASSERT_EQ(1u, functions.size());
    EXPECT_EQ("add", functions[0].name());

    client->dropFunction(db(), "add");
    EXPECT_TRUE(client->getFunctions(db()).empty());
}

TEST_F(LegacyMongoClientTests, managesUsers)
{
    ROBO_REQUIRE_MONGOD(server);
    MongoUser user(client->getVersion());
    user.setName("reporter");
    user.setPassword("reporter-pass");
    user.setUserSource(db());   // the Add User dialog sets the selected database
    user.setRoles({ "read" });

    client->createUser(db(), user);
    std::vector<MongoUser> users = client->getUsers(db());
    ASSERT_EQ(1u, users.size());
    EXPECT_EQ("reporter", users[0].name());

    client->dropUser(db(), "reporter");
    EXPECT_TRUE(client->getUsers(db()).empty());
}

TEST_F(LegacyMongoClientTests, loadsStorageStats)
{
    ROBO_REQUIRE_MONGOD(server);
    for (int i = 0; i < 50; ++i)
        client->insertDocument(BSON("_id" << i << "payload" << std::string(500, 'x')), MongoNamespace(db(), "sized"));

    MongoDatabaseStorageStats const stats = client->loadStorageStats(db());

    EXPECT_GT(stats.storageSizeBytes, 0);
    ASSERT_EQ(1u, stats.collections.size());
    EXPECT_EQ("sized", stats.collections[0].name);
    EXPECT_EQ(1u, stats.collections[0].indexSizes.size());
}
