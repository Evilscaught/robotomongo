#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QBrush>

#include "robomongo-unit-tests/QStringPrinter.h"
#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/widgets/workarea/BsonTableModel.h"
#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

using namespace Robomongo;

namespace
{
    // Same shape as BsonTreeModel's top level (root -> one item per document -> one item per
    // field) without BsonTreeModel's dependency on the application settings.
    class DocumentsModel : public QAbstractItemModel
    {
    public:
        explicit DocumentsModel(const std::vector<std::vector<std::pair<QString, QString>>> &docs)
            : _root(new BsonTreeItem(this))
        {
            for (auto const& fields : docs) {
                auto document = new BsonTreeItem(_root);
                document->setType(mongo::Object);
                for (auto const& field : fields) {
                    auto item = new BsonTreeItem(document);
                    item->setKey(field.first);
                    item->setValue(field.second);
                    item->setType(mongo::String);
                    document->addChild(item);
                }
                _root->addChild(document);
            }
        }

        BsonTreeItem *document(int row) const { return _root->child(row); }

        QModelIndex index(int row, int column, const QModelIndex &parent) const override
        {
            if (parent.isValid() || row < 0 || row >= rowCount(parent))
                return QModelIndex();
            return createIndex(row, column, _root->child(row));
        }
        QModelIndex parent(const QModelIndex &) const override { return QModelIndex(); }
        int rowCount(const QModelIndex &parent) const override
        {
            return parent.isValid() ? 0 : static_cast<int>(_root->childrenCount());
        }
        int columnCount(const QModelIndex &) const override { return BsonTreeItem::eCountColumns; }
        QVariant data(const QModelIndex &, int) const override { return QVariant(); }

    private:
        BsonTreeItem *const _root;
    };

    using Doc = std::vector<std::pair<QString, QString>>;
}

class BsonTableModelTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
};

TEST_F(BsonTableModelTests, columnsAreUnionOfFieldsInFirstSeenOrder)
{
    DocumentsModel source({ Doc{ { "_id", "1" }, { "name", "Ada" } },
                            Doc{ { "_id", "2" }, { "age", "36" }, { "name", "Bob" } } });
    BsonTableModelProxy proxy;

    proxy.setSourceModel(&source);

    ASSERT_EQ(3, proxy.columnCount(QModelIndex()));
    EXPECT_EQ(QString("_id"), proxy.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString());
    EXPECT_EQ(QString("name"), proxy.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString());
    EXPECT_EQ(QString("age"), proxy.headerData(2, Qt::Horizontal, Qt::DisplayRole).toString());
}

TEST_F(BsonTableModelTests, headersShowFieldNamesAndRowNumbers)
{
    DocumentsModel source({ Doc{ { "_id", "1" } }, Doc{ { "_id", "2" } } });
    BsonTableModelProxy proxy;
    proxy.setSourceModel(&source);

    EXPECT_EQ(QVariant("_id"), proxy.headerData(0, Qt::Horizontal, Qt::DisplayRole));
    EXPECT_EQ(QVariant("1"), proxy.headerData(0, Qt::Vertical, Qt::DisplayRole));
    EXPECT_EQ(QVariant("2"), proxy.headerData(1, Qt::Vertical, Qt::DisplayRole));
    EXPECT_FALSE(proxy.headerData(0, Qt::Horizontal, Qt::ToolTipRole).isValid());
}

TEST_F(BsonTableModelTests, rowPerDocument)
{
    DocumentsModel source({ Doc{ { "a", "1" } }, Doc{ { "a", "2" } }, Doc{ { "a", "3" } } });
    BsonTableModelProxy proxy;
    proxy.setSourceModel(&source);

    EXPECT_EQ(3, proxy.rowCount(QModelIndex()));
}

TEST_F(BsonTableModelTests, cellsShowFieldValues)
{
    DocumentsModel source({ Doc{ { "_id", "1" }, { "name", "Ada" } },
                            Doc{ { "_id", "2" }, { "name", "Bob" } } });
    BsonTableModelProxy proxy;
    proxy.setSourceModel(&source);

    EXPECT_EQ(QVariant("Ada"), proxy.data(proxy.index(0, 1, QModelIndex()), Qt::DisplayRole));
    EXPECT_EQ(QVariant("2"), proxy.data(proxy.index(1, 0, QModelIndex()), Qt::DisplayRole));
}

TEST_F(BsonTableModelTests, missingFieldsAreEmptyGreyCells)
{
    DocumentsModel source({ Doc{ { "_id", "1" }, { "name", "Ada" } }, Doc{ { "_id", "2" } } });
    BsonTableModelProxy proxy;
    proxy.setSourceModel(&source);

    QModelIndex const missing = proxy.index(1, 1, QModelIndex());

    EXPECT_FALSE(proxy.data(missing, Qt::DisplayRole).isValid());
    EXPECT_EQ(QBrush("#f5f3f2"), proxy.data(missing, Qt::BackgroundRole).value<QBrush>());
}

TEST_F(BsonTableModelTests, indexPointsAtFieldItem)
{
    DocumentsModel source({ Doc{ { "_id", "1" }, { "name", "Ada" } } });
    BsonTableModelProxy proxy;
    proxy.setSourceModel(&source);

    QModelIndex const cell = proxy.index(0, 1, QModelIndex());

    EXPECT_EQ(source.document(0)->childByKey("name"), QtUtils::item<BsonTreeItem *>(cell));
    EXPECT_FALSE(proxy.index(0, 5, QModelIndex()).isValid());
}

TEST_F(BsonTableModelTests, mapsCellsBackToDocuments)
{
    DocumentsModel source({ Doc{ { "_id", "1" } }, Doc{ { "_id", "2" }, { "name", "Bob" } } });
    BsonTableModelProxy proxy;
    proxy.setSourceModel(&source);

    QModelIndex const sourceIndex = proxy.mapToSource(proxy.index(1, 1, QModelIndex()));

    EXPECT_EQ(1, sourceIndex.row());
    EXPECT_EQ(source.document(1), QtUtils::item<BsonTreeItem *>(sourceIndex));
}

TEST_F(BsonTableModelTests, emptySourceHasNoColumns)
{
    DocumentsModel source({});
    BsonTableModelProxy proxy;

    proxy.setSourceModel(&source);

    EXPECT_EQ(0, proxy.columnCount(QModelIndex()));
    EXPECT_EQ(0, proxy.rowCount(QModelIndex()));
}

TEST_F(BsonTableModelTests, repeatedFieldsAreOneColumn)
{
    DocumentsModel source({ Doc{ { "a", "1" }, { "b", "2" } }, Doc{ { "b", "3" }, { "a", "4" } },
                            Doc{ { "a", "5" } } });
    BsonTableModelProxy proxy;

    proxy.setSourceModel(&source);

    ASSERT_EQ(2, proxy.columnCount(QModelIndex()));
    EXPECT_EQ(QVariant("4"), proxy.data(proxy.index(1, 0, QModelIndex()), Qt::DisplayRole));
    EXPECT_EQ(QVariant("3"), proxy.data(proxy.index(1, 1, QModelIndex()), Qt::DisplayRole));
}
