#include "gtest/gtest.h"

#include "robomongo-unit-tests/QStringPrinter.h"

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/QueryResultFormatter.h"
#include "robomongo/shell/bson/json.h"

#include <mongo/bson/bsonobjbuilder.h>

namespace
{
    std::vector<Robomongo::MongoDocumentPtr> makeDocuments()
    {
        std::vector<Robomongo::MongoDocumentPtr> docs;
        docs.push_back(Robomongo::MongoDocument::fromBsonObj(BSON("name" << "Ana" << "age" << 30)));
        docs.push_back(Robomongo::MongoDocument::fromBsonObj(BSON("name" << "Bob, Jr." << "active" << true)));
        return docs;
    }
}

TEST(QueryResultFormatterTests, formatsDocumentsAsJsonLines)
{
    const QString output = Robomongo::QueryResultFormatter::format(
        makeDocuments(), Robomongo::QueryResultExportFormat::JsonLines,
        Robomongo::DefaultEncoding, Robomongo::Utc);

    EXPECT_EQ(QString("{ \"name\" : \"Ana\", \"age\" : 30 }\n"
                      "{ \"name\" : \"Bob, Jr.\", \"active\" : true }\n"), output);
}

TEST(QueryResultFormatterTests, formatsDocumentsAsCsvWithEscapedCells)
{
    const QString output = Robomongo::QueryResultFormatter::format(
        makeDocuments(), Robomongo::QueryResultExportFormat::Csv,
        Robomongo::DefaultEncoding, Robomongo::Utc);

    EXPECT_EQ(QString("name,age,active\n"
                      "Ana,30,\n"
                      "\"Bob, Jr.\",,true\n"), output);
}

namespace
{
    std::vector<Robomongo::MongoDocumentPtr> documents(std::initializer_list<mongo::BSONObj> objs)
    {
        std::vector<Robomongo::MongoDocumentPtr> docs;
        for (auto const& obj : objs)
            docs.push_back(Robomongo::MongoDocument::fromBsonObj(obj));
        return docs;
    }

    QString exportAs(const std::vector<Robomongo::MongoDocumentPtr> &docs,
                     Robomongo::QueryResultExportFormat format,
                     Robomongo::UUIDEncoding uuid = Robomongo::DefaultEncoding)
    {
        return Robomongo::QueryResultFormatter::format(docs, format, uuid, Robomongo::Utc);
    }
}

TEST(QueryResultFormatterTests, formatsDocumentsAsPrettyJsonArray)
{
    QString const output = exportAs(makeDocuments(), Robomongo::QueryResultExportFormat::Json);

    EXPECT_EQ(QString("[\n"
                      "{\n    \"name\" : \"Ana\",\n    \"age\" : 30\n},\n"
                      "{\n    \"name\" : \"Bob, Jr.\",\n    \"active\" : true\n}\n"
                      "]\n"), output);
}

TEST(QueryResultFormatterTests, emptyResultsInEveryFormat)
{
    std::vector<Robomongo::MongoDocumentPtr> const none;

    EXPECT_EQ(QString("[\n]\n"), exportAs(none, Robomongo::QueryResultExportFormat::Json));
    EXPECT_EQ(QString(""), exportAs(none, Robomongo::QueryResultExportFormat::JsonLines));
    EXPECT_EQ(QString("\n"), exportAs(none, Robomongo::QueryResultExportFormat::Csv));
}

TEST(QueryResultFormatterTests, jsonLinesKeepsEachDocumentOnOneLine)
{
    auto const docs = documents({ BSON("s" << "line one\nline two" << "arr" << BSON_ARRAY(1 << 2)
                                           << "doc" << BSON("a" << BSON("b" << 1))) });

    QString const output = exportAs(docs, Robomongo::QueryResultExportFormat::JsonLines);

    EXPECT_EQ(1, output.count('\n'));
    EXPECT_TRUE(output.endsWith("\n"));
    EXPECT_TRUE(output.contains("line one\\nline two"));
}

TEST(QueryResultFormatterTests, jsonLinesCanBeParsedBack)
{
    auto const docs = documents({ BSON("_id" << 1 << "name" << "Ana" << "tags" << BSON_ARRAY("a" << "b")),
                                  BSON("_id" << 2 << "nested" << BSON("x" << 2.5)) });

    QStringList const lines = exportAs(docs, Robomongo::QueryResultExportFormat::JsonLines)
                                  .split('\n', QString::SkipEmptyParts);

    ASSERT_EQ(2, lines.size());
    for (int i = 0; i < lines.size(); ++i) {
        mongo::BSONObj const parsed = mongo::Robomongo::fromjson(lines[i].toStdString());
        EXPECT_EQ(0, docs[i]->bsonObj().woCompare(parsed)) << lines[i].toStdString();
    }
}

TEST(QueryResultFormatterTests, csvHeadersAreUnionInFirstSeenOrder)
{
    auto const docs = documents({ BSON("b" << 1 << "a" << 2), BSON("c" << 3 << "a" << 4) });

    QString const output = exportAs(docs, Robomongo::QueryResultExportFormat::Csv);

    EXPECT_EQ(QString("b,a,c\n1,2,\n,4,3\n"), output);
}

TEST(QueryResultFormatterTests, csvEscapesQuotesCommasAndNewlines)
{
    auto const docs = documents({ BSON("q" << "say \"hi\"" << "c" << "a,b" << "n" << "two\nlines"
                                           << "plain" << "ok") });

    QString const output = exportAs(docs, Robomongo::QueryResultExportFormat::Csv);

    EXPECT_EQ(QString("q,c,n,plain\n\"say \"\"hi\"\"\",\"a,b\",\"two\nlines\",ok\n"), output);
}

TEST(QueryResultFormatterTests, csvLeavesNullAndMissingCellsEmpty)
{
    mongo::BSONObjBuilder builder;
    builder.append("a", 1);
    builder.appendNull("b");
    auto const docs = documents({ builder.obj(), BSON("a" << 2) });

    EXPECT_EQ(QString("a,b\n1,\n2,\n"), exportAs(docs, Robomongo::QueryResultExportFormat::Csv));
}

TEST(QueryResultFormatterTests, csvWritesStringsUnquotedAndOtherValuesAsJson)
{
    auto const docs = documents({ BSON("s" << "text" << "n" << 2.5 << "l" << 5LL << "b" << false
                                           << "_id" << mongo::OID("5f1d7a2b3c4d5e6f70819203")) });

    QString const output = exportAs(docs, Robomongo::QueryResultExportFormat::Csv);

    // ObjectId(...) contains quotes, so the cell is quoted and its quotes doubled
    EXPECT_EQ(QString("s,n,l,b,_id\n"
                      "text,2.5,NumberLong(5),false,\"ObjectId(\"\"5f1d7a2b3c4d5e6f70819203\"\")\"\n"),
              output);
}

TEST(QueryResultFormatterTests, csvUsesChosenUuidEncoding)
{
    unsigned char const bytes[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                      0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
    mongo::BSONObjBuilder builder;
    builder.appendBinData("u", 16, mongo::bdtUUID, bytes);
    auto const docs = documents({ builder.obj() });

    QString const output = exportAs(docs, Robomongo::QueryResultExportFormat::Csv, Robomongo::JavaLegacy);

    // The cell contains quotes, so it is quoted and its quotes doubled
    EXPECT_EQ(QString("u\n\"JUUID(\"\"77665544-3322-1100-ffee-ddccbbaa9988\"\")\"\n"), output);
}

TEST(QueryResultFormatterTests, csvKeepsUnicodeFieldNamesAndValues)
{
    auto const docs = documents({ BSON("名前" << "Ünïcødé") });

    EXPECT_EQ(QString::fromUtf8("名前\nÜnïcødé\n"), exportAs(docs, Robomongo::QueryResultExportFormat::Csv));
}
