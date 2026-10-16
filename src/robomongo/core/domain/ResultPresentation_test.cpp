#include <gtest/gtest.h>

#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/ResultPresentation.h"

using namespace Robomongo;

namespace
{
    MongoQueryInfo findOn(const std::string &collection)
    {
        return MongoQueryInfo(CollectionInfo("localhost:27017", "shop", collection),
                              mongo::BSONObj(), mongo::BSONObj(), 0, 0, 50, 0, false);
    }

    std::vector<MongoDocumentPtr> oneDocument()
    {
        return { MongoDocument::fromBsonObj(BSON("_id" << 1)) };
    }
}

TEST(ResultPresentationTests, emptyFindUsesDocumentView)
{
    MongoShellResult const result("", "", {}, findOn("users"), "db.users.find({})", 0);

    EXPECT_TRUE(ResultPresentation::usesDocumentView(result));
}

TEST(ResultPresentationTests, findWithDocumentsUsesDocumentView)
{
    MongoShellResult const result("", "", oneDocument(), findOn("users"), "db.users.find({})", 0);

    EXPECT_TRUE(ResultPresentation::usesDocumentView(result));
}

TEST(ResultPresentationTests, documentsWithoutFindUseDocumentView)
{
    // e.g. db.stats() returns a document but is not a find on a collection
    MongoShellResult const result("", "", oneDocument(), MongoQueryInfo(), "db.stats()", 0);

    EXPECT_TRUE(ResultPresentation::usesDocumentView(result));
}

TEST(ResultPresentationTests, plainOutputWithoutDocumentsIsText)
{
    MongoShellResult const result("", "hello", {}, MongoQueryInfo(), "print('hello')", 0);

    EXPECT_FALSE(ResultPresentation::usesDocumentView(result));
}
