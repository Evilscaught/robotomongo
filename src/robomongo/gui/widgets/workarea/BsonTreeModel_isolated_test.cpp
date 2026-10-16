// Runs in robo_isolated_tests (HOME is a scratch directory; see IsolatedMain.cpp)

#include <gtest/gtest.h>

#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/widgets/workarea/BsonTreeModel.h"

using namespace Robomongo;

namespace
{
    std::vector<MongoDocumentPtr> documents(std::initializer_list<mongo::BSONObj> objs)
    {
        std::vector<MongoDocumentPtr> docs;
        for (auto const& obj : objs)
            docs.push_back(MongoDocument::fromBsonObj(obj));
        return docs;
    }

    QString text(const BsonTreeModel &model, const QModelIndex &index, int column,
                 int role = Qt::DisplayRole)
    {
        return model.data(index.sibling(index.row(), column), role).toString();
    }

    QModelIndex childByKey(BsonTreeModel &model, const QModelIndex &parent, const QString &key)
    {
        if (model.canFetchMore(parent))
            model.fetchMore(parent);
        for (int row = 0; row < model.rowCount(parent); ++row) {
            QModelIndex const child = model.index(row, 0, parent);
            if (model.data(child, Qt::DisplayRole).toString() == key)
                return child;
        }
        return QModelIndex();
    }
}

class BsonTreeModelTests : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ensureQtTestApplication();
        AppRegistry::instance().settingsManager()->setUuidEncoding(DefaultEncoding);
        AppRegistry::instance().settingsManager()->setTimeZone(Utc);
    }
};

TEST_F(BsonTreeModelTests, oneRowPerDocumentWithThreeColumns)
{
    BsonTreeModel model(documents({ BSON("_id" << 1), BSON("_id" << 2), BSON("_id" << 3) }));

    EXPECT_EQ(3, model.rowCount(QModelIndex()));
    EXPECT_EQ(3, model.columnCount(QModelIndex()));
}

TEST_F(BsonTreeModelTests, headersAreKeyValueType)
{
    BsonTreeModel model(documents({}));

    EXPECT_EQ(QVariant("Key"), model.headerData(0, Qt::Horizontal, Qt::DisplayRole));
    EXPECT_EQ(QVariant("Value"), model.headerData(1, Qt::Horizontal, Qt::DisplayRole));
    EXPECT_EQ(QVariant("Type"), model.headerData(2, Qt::Horizontal, Qt::DisplayRole));
}

TEST_F(BsonTreeModelTests, documentRowsShowNumberIdAndFieldCount)
{
    BsonTreeModel model(documents({ BSON("_id" << mongo::OID("5f1d7a2b3c4d5e6f70819203") << "a" << 1 << "b" << 2),
                                    BSON("x" << 1) }));
    QModelIndex const first = model.index(0, 0, QModelIndex());
    QModelIndex const second = model.index(1, 0, QModelIndex());

    EXPECT_EQ(QString("(1) ObjectId(\"5f1d7a2b3c4d5e6f70819203\")"), text(model, first, 0));
    EXPECT_EQ(QString("{ 3 fields }"), text(model, first, 1));
    EXPECT_EQ(QString("Object"), text(model, first, 2));
    EXPECT_EQ(QString("(2) "), text(model, second, 0));
    EXPECT_EQ(QString("{ 1 field }"), text(model, second, 1));
}

TEST_F(BsonTreeModelTests, fieldsShowNameValueAndType)
{
    BsonTreeModel model(documents({ BSON("_id" << 1 << "name" << "Ada" << "age" << 36 << "score" << 2.5
                                               << "big" << 5000000000LL << "ok" << true) }));
    QModelIndex const doc = model.index(0, 0, QModelIndex());

    QModelIndex const name = childByKey(model, doc, "name");
    QModelIndex const age = childByKey(model, doc, "age");
    QModelIndex const score = childByKey(model, doc, "score");
    QModelIndex const big = childByKey(model, doc, "big");
    QModelIndex const ok = childByKey(model, doc, "ok");

    EXPECT_EQ(QString("Ada"), text(model, name, 1));
    EXPECT_EQ(QString("String"), text(model, name, 2));
    EXPECT_EQ(QString("36"), text(model, age, 1));
    EXPECT_EQ(QString("Int32"), text(model, age, 2));
    EXPECT_EQ(QString("2.5"), text(model, score, 1));
    EXPECT_EQ(QString("Double"), text(model, score, 2));
    EXPECT_EQ(QString("5000000000"), text(model, big, 1));
    EXPECT_EQ(QString("Int64"), text(model, big, 2));
    EXPECT_EQ(QString("true"), text(model, ok, 1));
    EXPECT_EQ(QString("Boolean"), text(model, ok, 2));
}

TEST_F(BsonTreeModelTests, nestedDocumentsAndArraysLoadOnDemand)
{
    BsonTreeModel model(documents({ BSON("_id" << 1 << "address" << BSON("city" << "Paris" << "zip" << "75001")
                                               << "tags" << BSON_ARRAY("a" << "b" << "c")) }));
    QModelIndex const doc = model.index(0, 0, QModelIndex());
    QModelIndex const address = childByKey(model, doc, "address");
    QModelIndex const tags = childByKey(model, doc, "tags");

    EXPECT_EQ(QString("{ 2 fields }"), text(model, address, 1));
    EXPECT_EQ(QString("[ 3 elements ]"), text(model, tags, 1));
    EXPECT_EQ(QString("Array"), text(model, tags, 2));
    EXPECT_TRUE(model.hasChildren(address));
    EXPECT_TRUE(model.canFetchMore(address));
    EXPECT_EQ(0, model.rowCount(address));

    model.fetchMore(address);
    model.fetchMore(tags);

    EXPECT_FALSE(model.canFetchMore(address));
    EXPECT_EQ(2, model.rowCount(address));
    EXPECT_EQ(QString("Paris"), text(model, childByKey(model, address, "city"), 1));
    EXPECT_EQ(3, model.rowCount(tags));
    EXPECT_EQ(QString("[0]"), text(model, model.index(0, 0, tags), 0));
    EXPECT_EQ(QString("c"), text(model, model.index(2, 0, tags), 1));
}

TEST_F(BsonTreeModelTests, singularAndPluralCounts)
{
    BsonTreeModel model(documents({ BSON("one" << BSON_ARRAY(1) << "none" << mongo::BSONArray()
                                               << "single" << BSON("a" << 1) << "empty" << mongo::BSONObj()) }));
    QModelIndex const doc = model.index(0, 0, QModelIndex());

    EXPECT_EQ(QString("[ 1 element ]"), text(model, childByKey(model, doc, "one"), 1));
    EXPECT_EQ(QString("[ 0 elements ]"), text(model, childByKey(model, doc, "none"), 1));
    EXPECT_EQ(QString("{ 1 field }"), text(model, childByKey(model, doc, "single"), 1));
    EXPECT_EQ(QString("{ 0 fields }"), text(model, childByKey(model, doc, "empty"), 1));
}

TEST_F(BsonTreeModelTests, scalarsHaveNoChildren)
{
    BsonTreeModel model(documents({ BSON("name" << "Ada") }));
    QModelIndex const doc = model.index(0, 0, QModelIndex());
    QModelIndex const name = childByKey(model, doc, "name");

    EXPECT_FALSE(model.hasChildren(name));
    EXPECT_FALSE(model.canFetchMore(name));
}

TEST_F(BsonTreeModelTests, longStringsAreShortenedForDisplay)
{
    BsonTreeModel model(documents({ BSON("text" << std::string(2000, 'x')) }));
    QModelIndex const field = childByKey(model, model.index(0, 0, QModelIndex()), "text");

    EXPECT_EQ(300, text(model, field, 1, Qt::DisplayRole).size());
    EXPECT_EQ(500, text(model, field, 1, Qt::ToolTipRole).size());
}

TEST_F(BsonTreeModelTests, displayedStringsAreOnOneLine)
{
    BsonTreeModel model(documents({ BSON("text" << "line one\nline two\n\nline   three") }));
    QModelIndex const field = childByKey(model, model.index(0, 0, QModelIndex()), "text");

    EXPECT_EQ(QString("line one line two line three"), text(model, field, 1));
}

TEST_F(BsonTreeModelTests, parentAndIndexAreConsistent)
{
    BsonTreeModel model(documents({ BSON("a" << 1), BSON("b" << BSON("c" << 2)) }));
    QModelIndex const doc = model.index(1, 0, QModelIndex());
    QModelIndex const b = childByKey(model, doc, "b");
    model.fetchMore(b);
    QModelIndex const c = model.index(0, 0, b);

    EXPECT_FALSE(model.parent(doc).isValid());
    EXPECT_EQ(doc, model.parent(b));
    EXPECT_EQ(b, model.parent(c));
    EXPECT_EQ(1, model.parent(b).row());
}

TEST_F(BsonTreeModelTests, itemsAreSelectableButNotEditable)
{
    BsonTreeModel model(documents({ BSON("a" << 1) }));
    Qt::ItemFlags const flags = model.flags(model.index(0, 0, QModelIndex()));

    EXPECT_TRUE(flags & Qt::ItemIsSelectable);
    EXPECT_TRUE(flags & Qt::ItemIsEnabled);
    EXPECT_FALSE(flags & Qt::ItemIsEditable);
    EXPECT_EQ(Qt::ItemFlags(), model.flags(QModelIndex()));
}

TEST_F(BsonTreeModelTests, outOfRangeIndexesAreInvalid)
{
    BsonTreeModel model(documents({ BSON("a" << 1) }));

    EXPECT_FALSE(model.index(5, 0, QModelIndex()).isValid());
    EXPECT_FALSE(model.index(0, 7, QModelIndex()).isValid());
    EXPECT_FALSE(model.data(QModelIndex(), Qt::DisplayRole).isValid());
}

TEST_F(BsonTreeModelTests, fieldItemsKeepTheirDocument)
{
    mongo::BSONObj const doc = BSON("_id" << 7 << "user" << BSON("name" << "Ada"));
    BsonTreeModel model(documents({ doc }));
    QModelIndex const user = childByKey(model, model.index(0, 0, QModelIndex()), "user");
    model.fetchMore(user);
    auto name = static_cast<BsonTreeItem *>(model.index(0, 0, user).internalPointer());

    EXPECT_EQ(doc.toString(), name->superRoot().toString());
    EXPECT_EQ("name", name->fieldName());
}
