#include <gtest/gtest.h>

#include <QPointer>

#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

using namespace Robomongo;

namespace
{
    BsonTreeItem *child(BsonTreeItem *parent, const QString &key, mongo::BSONObj root = mongo::BSONObj())
    {
        auto item = new BsonTreeItem(root, parent);
        item->setKey(key);
        parent->addChild(item);
        return item;
    }
}

TEST(BsonTreeItemTests, keepsFields)
{
    BsonTreeItem item;
    item.setKey("name");
    item.setValue("Ada");
    item.setType(mongo::String);
    item.setBinType(mongo::newUUID);
    item.setFieldName("name");

    EXPECT_EQ(QString("name"), item.key());
    EXPECT_EQ(QString("Ada"), item.value());
    EXPECT_EQ(mongo::String, item.type());
    EXPECT_EQ(mongo::newUUID, item.binType());
    EXPECT_EQ("name", item.fieldName());
}

TEST(BsonTreeItemTests, keepsRootDocument)
{
    mongo::BSONObj const doc = BSON("_id" << 1 << "name" << "Ada");

    BsonTreeItem const item(doc);

    EXPECT_EQ(doc.toString(), item.root().toString());
}

TEST(BsonTreeItemTests, addsAndFindsChildren)
{
    BsonTreeItem parent;
    BsonTreeItem *a = child(&parent, "a");
    BsonTreeItem *b = child(&parent, "b");

    EXPECT_EQ(2u, parent.childrenCount());
    EXPECT_EQ(a, parent.child(0));
    EXPECT_EQ(b, parent.child(1));
    EXPECT_EQ(1, parent.indexOf(b));
    EXPECT_EQ(b, parent.childByKey("b"));
}

TEST(BsonTreeItemTests, missingChildrenGiveNull)
{
    BsonTreeItem parent;
    child(&parent, "a");
    BsonTreeItem other;

    EXPECT_EQ(nullptr, parent.childSafe(1));
    EXPECT_EQ(nullptr, parent.childByKey("missing"));
    EXPECT_EQ(-1, parent.indexOf(&other));
}

TEST(BsonTreeItemTests, childByKeyReturnsFirstMatch)
{
    BsonTreeItem parent;
    BsonTreeItem *first = child(&parent, "dup");
    child(&parent, "dup");

    EXPECT_EQ(first, parent.childByKey("dup"));
}

TEST(BsonTreeItemTests, removeChildDeletesIt)
{
    BsonTreeItem parent;
    child(&parent, "a");
    QPointer<BsonTreeItem> const b = child(&parent, "b");
    child(&parent, "c");

    parent.removeChild(b);

    EXPECT_EQ(2u, parent.childrenCount());
    EXPECT_EQ(QString("a"), parent.child(0)->key());
    EXPECT_EQ(QString("c"), parent.child(1)->key());
    EXPECT_TRUE(b.isNull());
}

TEST(BsonTreeItemTests, clearForgetsChildren)
{
    BsonTreeItem parent;
    child(&parent, "a");
    child(&parent, "b");

    parent.clear();

    EXPECT_EQ(0u, parent.childrenCount());
}

TEST(BsonTreeItemTests, superParentIsTheDocumentItem)
{
    // Model root -> document -> field -> nested field
    mongo::BSONObj const doc = BSON("_id" << 1 << "address" << BSON("city" << "Paris"));
    BsonTreeItem modelRoot;
    BsonTreeItem *document = child(&modelRoot, "(1) 1", doc);
    BsonTreeItem *address = child(document, "address", doc);
    BsonTreeItem *city = child(address, "city", doc.getObjectField("address"));

    EXPECT_EQ(document, city->superParent());
    EXPECT_EQ(document, address->superParent());
    EXPECT_EQ(document, document->superParent());
    EXPECT_EQ(doc.toString(), city->superRoot().toString());
}
