#include <gtest/gtest.h>

#include <QAbstractItemModel>

#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/core/domain/Notifier.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

using namespace Robomongo;

// Defined in Notifier.cpp but not declared in its header
namespace Robomongo
{
    namespace detail
    {
        bool isArrayChild(BsonTreeItem const *item);
        bool isDocumentRoot(BsonTreeItem const *item);
    }
}

namespace
{
    // Results tree like BsonTreeModel: root -> documents -> fields -> nested fields
    class ResultsTree : public QAbstractItemModel
    {
    public:
        ResultsTree() : _root(new BsonTreeItem(this)) {}

        BsonTreeItem *add(BsonTreeItem *parent, const QString &key, mongo::BSONType type)
        {
            auto item = new BsonTreeItem(parent ? static_cast<QObject *>(parent) : _root);
            item->setKey(key);
            item->setType(type);
            (parent ? parent : _root)->addChild(item);
            return item;
        }

        QModelIndex indexOf(BsonTreeItem *item) const
        {
            auto parent = static_cast<BsonTreeItem *>(item->parent());
            return createIndex(parent->indexOf(item), 0, item);
        }

        QModelIndex index(int row, int column, const QModelIndex &parent) const override
        {
            BsonTreeItem *parentItem = parent.isValid() ? QtUtils::item<BsonTreeItem *>(parent) : _root;
            BsonTreeItem *child = parentItem->childSafe(row);
            return child ? createIndex(row, column, child) : QModelIndex();
        }

        QModelIndex parent(const QModelIndex &index) const override
        {
            auto item = QtUtils::item<BsonTreeItem *>(index);
            auto parentItem = item ? static_cast<BsonTreeItem *>(item->parent()) : nullptr;
            if (!parentItem || parentItem == _root)
                return QModelIndex();
            auto grandParent = static_cast<BsonTreeItem *>(parentItem->parent());
            return createIndex(grandParent->indexOf(parentItem), 0, parentItem);
        }

        int rowCount(const QModelIndex &parent) const override
        {
            BsonTreeItem *item = parent.isValid() ? QtUtils::item<BsonTreeItem *>(parent) : _root;
            return static_cast<int>(item->childrenCount());
        }
        int columnCount(const QModelIndex &) const override { return 3; }
        QVariant data(const QModelIndex &, int) const override { return QVariant(); }

    private:
        BsonTreeItem *const _root;
    };

    BsonTreeItem *item(const QModelIndex &index) { return QtUtils::item<BsonTreeItem *>(index); }
}

class NotifierSelectionTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        doc1 = tree.add(nullptr, "(1) 1", mongo::Object);
        doc1Name = tree.add(doc1, "name", mongo::String);
        doc1Tags = tree.add(doc1, "tags", mongo::Array);
        doc1Tag0 = tree.add(doc1Tags, "[0]", mongo::String);
        doc1Address = tree.add(doc1, "address", mongo::Object);
        doc1City = tree.add(doc1Address, "city", mongo::String);
        doc2 = tree.add(nullptr, "(2) 2", mongo::Object);
        doc2Name = tree.add(doc2, "name", mongo::String);
        doc3 = tree.add(nullptr, "(3) 3", mongo::Object);
    }

    ResultsTree tree;
    BsonTreeItem *doc1, *doc1Name, *doc1Tags, *doc1Tag0, *doc1Address, *doc1City;
    BsonTreeItem *doc2, *doc2Name, *doc3;
};

TEST_F(NotifierSelectionTests, uniqueRowsKeepsOneIndexPerDocument)
{
    QModelIndexList const selected { tree.indexOf(doc1Name), tree.indexOf(doc1City),
                                     tree.indexOf(doc2Name), tree.indexOf(doc1Tag0) };

    QModelIndexList const unique = detail::uniqueRows(selected);

    ASSERT_EQ(2, unique.size());
    EXPECT_EQ(doc1Name, item(unique[0]));
    EXPECT_EQ(doc2Name, item(unique[1]));
}

TEST_F(NotifierSelectionTests, uniqueRowsCanReturnDocuments)
{
    QModelIndexList const selected { tree.indexOf(doc1City), tree.indexOf(doc2Name), tree.indexOf(doc3) };

    QModelIndexList const documents = detail::uniqueRows(selected, true);

    ASSERT_EQ(3, documents.size());
    EXPECT_EQ(doc1, item(documents[0]));
    EXPECT_EQ(doc2, item(documents[1]));
    EXPECT_EQ(doc3, item(documents[2]));
}

TEST_F(NotifierSelectionTests, selectingEveryColumnOfARowCountsOnce)
{
    // A tree view selection holds one index per column of each selected row
    QModelIndex const name = tree.indexOf(doc2Name);
    QModelIndexList const selected { name, name.sibling(name.row(), 1), name.sibling(name.row(), 2) };

    EXPECT_EQ(1, detail::uniqueRows(selected, true).size());
}

TEST_F(NotifierSelectionTests, uniqueRowsIgnoresInvalidIndexes)
{
    QModelIndexList const selected { QModelIndex(), tree.indexOf(doc3), QModelIndex() };

    QModelIndexList const unique = detail::uniqueRows(selected, true);

    ASSERT_EQ(1, unique.size());
    EXPECT_EQ(doc3, item(unique[0]));
}

TEST_F(NotifierSelectionTests, uniqueRowsOfNothingIsEmpty)
{
    EXPECT_TRUE(detail::uniqueRows(QModelIndexList()).isEmpty());
}

TEST_F(NotifierSelectionTests, multiSelectionMeansMoreThanOneIndex)
{
    EXPECT_FALSE(detail::isMultiSelection(QModelIndexList()));
    EXPECT_FALSE(detail::isMultiSelection({ tree.indexOf(doc1) }));
    EXPECT_TRUE(detail::isMultiSelection({ tree.indexOf(doc1), tree.indexOf(doc2) }));
}

TEST_F(NotifierSelectionTests, classifiesItemTypes)
{
    BsonTreeItem *id = tree.add(doc3, "_id", mongo::jstOID);
    BsonTreeItem *uuid = tree.add(doc3, "uuid", mongo::BinData);
    uuid->setBinType(mongo::newUUID);
    BsonTreeItem *blob = tree.add(doc3, "blob", mongo::BinData);
    blob->setBinType(mongo::BinDataGeneral);

    EXPECT_TRUE(detail::isSimpleType(doc1Name));
    EXPECT_TRUE(detail::isSimpleType(id));
    EXPECT_TRUE(detail::isSimpleType(uuid));
    EXPECT_FALSE(detail::isSimpleType(blob));
    EXPECT_FALSE(detail::isSimpleType(doc1Address));
    EXPECT_TRUE(detail::isObjectIdType(id));
    EXPECT_FALSE(detail::isObjectIdType(doc1Name));
    EXPECT_TRUE(detail::isDocumentType(doc1Address));
    EXPECT_TRUE(detail::isDocumentType(doc1Tags));
    EXPECT_FALSE(detail::isDocumentType(doc1Name));
}

TEST_F(NotifierSelectionTests, recognisesArrayElements)
{
    EXPECT_TRUE(detail::isArrayChild(doc1Tag0));
    EXPECT_FALSE(detail::isArrayChild(doc1Name));
    EXPECT_FALSE(detail::isArrayChild(doc1City));
}

TEST_F(NotifierSelectionTests, recognisesDocumentRoots)
{
    EXPECT_TRUE(detail::isDocumentRoot(doc1));
    EXPECT_TRUE(detail::isDocumentRoot(doc3));
    EXPECT_FALSE(detail::isDocumentRoot(doc1Name));
    EXPECT_FALSE(detail::isDocumentRoot(doc1City));
}
