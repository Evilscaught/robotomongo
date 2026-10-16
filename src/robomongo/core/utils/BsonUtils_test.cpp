#include <gtest/gtest.h>

#include <QString>
#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/shell/bson/json.h"

using namespace Robomongo;

namespace
{
    // Text shown in the document editor (Edit Document / View Document)
    std::string editorText(const mongo::BSONObj &obj, SupportedTimes tz = Utc,
                           UUIDEncoding uuid = DefaultEncoding)
    {
        return BsonUtils::jsonString(obj, mongo::TenGen, 1, uuid, tz);
    }

    // Open a document in the editor and save it unchanged
    mongo::BSONObj editRoundTrip(const mongo::BSONObj &obj, SupportedTimes tz = Utc,
                                 UUIDEncoding uuid = DefaultEncoding)
    {
        return mongo::Robomongo::fromjson(editorText(obj, tz, uuid));
    }

    void expectUnchanged(const mongo::BSONObj &original, const mongo::BSONObj &saved)
    {
        mongo::BSONObjIterator it(original);
        while (it.more()) {
            mongo::BSONElement const before = it.next();
            mongo::BSONElement const after = saved.getField(before.fieldName());
            ASSERT_FALSE(after.eoo()) << before.fieldName() << " missing";
            EXPECT_EQ(before.type(), after.type()) << before.fieldName();
            EXPECT_TRUE(before.binaryEqualValues(after))
                << before.fieldName() << ": " << before.toString(false) << " -> " << after.toString(false);
        }
        EXPECT_EQ(original.nFields(), saved.nFields());
    }

    std::string displayed(const mongo::BSONElement &element, SupportedTimes tz = Utc,
                          UUIDEncoding uuid = DefaultEncoding)
    {
        std::string text;
        BsonUtils::buildJsonString(element, text, uuid, tz);
        return text;
    }

    mongo::BSONObj withUuid(mongo::BinDataType type)
    {
        unsigned char const bytes[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
        mongo::BSONObjBuilder builder;
        builder.appendBinData("u", sizeof(bytes), type, bytes);
        return builder.obj();
    }
}

// ---- Document editor round trip (open, then save unchanged) ----

TEST(BsonUtilsEditTests, keepsCommonTypes)
{
    mongo::BSONObjBuilder builder;
    builder.append("_id", mongo::OID("5f1d7a2b3c4d5e6f70819203"));
    builder.append("name", "Ada");
    builder.append("i", 7);
    builder.append("negative", std::numeric_limits<int>::min());
    builder.append("l", 5LL);
    builder.append("bigLong", 9223372036854775807LL);
    builder.append("price", mongo::Decimal128("19.990"));
    builder.append("yes", true);
    builder.appendNull("nothing");
    builder.appendUndefined("undef");
    mongo::BSONObj const original = builder.obj();

    expectUnchanged(original, editRoundTrip(original));
}

TEST(BsonUtilsEditTests, keepsDoublesThatNeedFewDigits)
{
    mongo::BSONObj const original = BSON("whole" << 5.0 << "half" << 2.5 << "neg" << -0.125
                                                 << "zero" << 0.0 << "big" << 1e15 << "big16" << 1.5e16
                                                 << "tiny" << 1e-7);

    mongo::BSONObj const saved = editRoundTrip(original);

    expectUnchanged(original, saved);
}

TEST(BsonUtilsEditTests, keepsWholeDoublesAsDoubles)
{
    std::string const text = editorText(BSON("whole" << 5.0 << "big" << 1e15));

    EXPECT_NE(std::string::npos, text.find("\"whole\" : 5.0"));
    EXPECT_NE(std::string::npos, text.find("\"big\" : 1000000000000000.0"));
}

TEST(BsonUtilsEditTests, keepsSpecialDoubles)
{
    mongo::BSONObj const original = BSON("inf" << std::numeric_limits<double>::infinity()
                                               << "ninf" << -std::numeric_limits<double>::infinity());

    mongo::BSONObj const saved = editRoundTrip(original);

    expectUnchanged(original, saved);
    double const nan = editRoundTrip(BSON("nan" << std::numeric_limits<double>::quiet_NaN()))
                           .getField("nan").Double();
    EXPECT_TRUE(std::isnan(nan));
}

TEST(BsonUtilsEditTests, keepsDatesInUtcAndLocalTime)
{
    mongo::BSONObjBuilder builder;
    builder.appendDate("now", mongo::Date_t::fromMillisSinceEpoch(1700000000123LL));
    builder.appendDate("epoch", mongo::Date_t::fromMillisSinceEpoch(0));
    builder.appendDate("old", mongo::Date_t::fromMillisSinceEpoch(-2208988800000LL));
    mongo::BSONObj const original = builder.obj();

    expectUnchanged(original, editRoundTrip(original, Utc));
    expectUnchanged(original, editRoundTrip(original, LocalTime));
}

TEST(BsonUtilsEditTests, showsUtcDatesAsIsoDate)
{
    mongo::BSONObjBuilder builder;
    builder.appendDate("d", mongo::Date_t::fromMillisSinceEpoch(1700000000123LL));

    EXPECT_NE(std::string::npos, editorText(builder.obj()).find("ISODate(\"2023-11-14T22:13:20.123Z\")"));
}

TEST(BsonUtilsEditTests, keepsBinaryTimestampAndKeys)
{
    unsigned char const bytes[] = { 0x00, 0x01, 0xfe, 0xff };
    mongo::BSONObjBuilder builder;
    builder.appendBinData("bin", sizeof(bytes), mongo::BinDataGeneral, bytes);
    builder.append("ts", mongo::Timestamp(1700000000, 7));
    builder.appendMinKey("min");
    builder.appendMaxKey("max");
    mongo::BSONObj const original = builder.obj();

    expectUnchanged(original, editRoundTrip(original));
}

TEST(BsonUtilsEditTests, keepsUuidsInEveryEncoding)
{
    for (UUIDEncoding encoding : { DefaultEncoding, JavaLegacy, CSharpLegacy, PythonLegacy }) {
        mongo::BSONObj const standard = withUuid(mongo::newUUID);
        mongo::BSONObj const legacy = withUuid(mongo::bdtUUID);

        expectUnchanged(standard, editRoundTrip(standard, Utc, encoding));
        expectUnchanged(legacy, editRoundTrip(legacy, Utc, encoding));
    }
}

TEST(BsonUtilsEditTests, keepsRegexWithIgnoreCaseAndMultiline)
{
    mongo::BSONObjBuilder builder;
    builder.appendRegex("re", "^a.b\\d$", "im");
    mongo::BSONObj const original = builder.obj();

    expectUnchanged(original, editRoundTrip(original));
}

TEST(BsonUtilsEditTests, keepsStringsWithEscapesAndUnicode)
{
    mongo::BSONObj const original = BSON("s" << "quote \" backslash \\ slash / tab \t newline \n é 🚀"
                                             << "empty" << "");

    expectUnchanged(original, editRoundTrip(original));
}

TEST(BsonUtilsEditTests, keepsNestedDocumentsAndArrays)
{
    mongo::BSONObj const original = BSON(
        "user" << BSON("name" << "Ada" << "tags" << BSON_ARRAY("a" << 2 << 3.5))
               << "matrix" << BSON_ARRAY(BSON_ARRAY(1 << 2) << BSON_ARRAY(3 << 4))
               << "emptyDoc" << mongo::BSONObj()
               << "emptyArray" << mongo::BSONArray());

    mongo::BSONObj const saved = editRoundTrip(original);

    EXPECT_EQ(0, original.woCompare(saved, mongo::BSONObj(), true));
    EXPECT_EQ(mongo::Array, saved.getField("emptyArray").type());
}

TEST(BsonUtilsEditTests, keepsFieldOrderAndUnusualNames)
{
    mongo::BSONObj const original = BSON("z" << 1 << "with space" << 2 << "ünïcode" << 3 << "a\"q" << 4);

    mongo::BSONObj const saved = editRoundTrip(original);

    EXPECT_EQ(original.toString(), saved.toString());
}

TEST(BsonUtilsEditTests, prettyPrintsWithFourSpaceIndent)
{
    std::string const text = editorText(BSON("a" << 1 << "b" << BSON("c" << 2)));

    EXPECT_EQ("{\n    \"a\" : 1,\n    \"b\" : {\n        \"c\" : 2\n    }\n}", text);
}

TEST(BsonUtilsEditTests, emptyDocument)
{
    EXPECT_TRUE(editRoundTrip(mongo::BSONObj()).isEmpty());
}

// ---- Strict format ----

TEST(BsonUtilsStrictTests, usesExtendedJsonWrappers)
{
    mongo::BSONObjBuilder builder;
    builder.append("_id", mongo::OID("5f1d7a2b3c4d5e6f70819203"));
    builder.append("ts", mongo::Timestamp(5, 6));
    builder.appendRegex("re", "a", "i");

    std::string const text = BsonUtils::jsonString(builder.obj(), mongo::Strict, 0, DefaultEncoding, Utc);

    EXPECT_NE(std::string::npos, text.find("{ \"$oid\" : \"5f1d7a2b3c4d5e6f70819203\" }"));
    EXPECT_NE(std::string::npos, text.find("{ \"$timestamp\" : { \"t\" : 5, \"i\" : 6 } }"));
    EXPECT_NE(std::string::npos, text.find("{ \"$regex\" : \"a\", \"$options\" : \"i\" }"));
}

// ---- Value display in the tree view ----

TEST(BsonUtilsDisplayTests, showsScalarValues)
{
    mongo::BSONObj const obj = BSON("s" << "text" << "i" << 42 << "l" << 5000000000LL << "d" << 2.5
                                        << "whole" << 3.0 << "t" << true << "dec" << mongo::Decimal128("1.10"));

    EXPECT_EQ("text", displayed(obj.getField("s")));
    EXPECT_EQ("42", displayed(obj.getField("i")));
    EXPECT_EQ("5000000000", displayed(obj.getField("l")));
    EXPECT_EQ("2.5", displayed(obj.getField("d")));
    EXPECT_EQ("3.0", displayed(obj.getField("whole")));
    EXPECT_EQ("true", displayed(obj.getField("t")));
    EXPECT_EQ("1.10", displayed(obj.getField("dec")));
}

TEST(BsonUtilsDisplayTests, showsObjectIdNullAndUndefined)
{
    mongo::BSONObjBuilder builder;
    builder.append("_id", mongo::OID("5f1d7a2b3c4d5e6f70819203"));
    builder.appendNull("n");
    builder.appendUndefined("u");
    mongo::BSONObj const obj = builder.obj();

    EXPECT_EQ("ObjectId(\"5f1d7a2b3c4d5e6f70819203\")", displayed(obj.getField("_id")));
    EXPECT_EQ("null", displayed(obj.getField("n")));
    EXPECT_EQ("undefined", displayed(obj.getField("u")));
}

TEST(BsonUtilsDisplayTests, showsDatesInChosenTimeZone)
{
    mongo::BSONObjBuilder builder;
    builder.appendDate("d", mongo::Date_t::fromMillisSinceEpoch(1700000000123LL));
    mongo::BSONObj const obj = builder.obj();

    EXPECT_EQ("2023-11-14 22:13:20.123Z", displayed(obj.getField("d"), Utc));
    std::string const local = displayed(obj.getField("d"), LocalTime);
    EXPECT_NE('Z', local.back());
}

TEST(BsonUtilsDisplayTests, showsBinaryPlaceholderAndUuids)
{
    unsigned char const bytes[] = { 1, 2, 3 };
    mongo::BSONObjBuilder builder;
    builder.appendBinData("b", 3, mongo::BinDataGeneral, bytes);
    mongo::BSONObj const obj = builder.obj();

    EXPECT_EQ("<binary>", displayed(obj.getField("b")));
    EXPECT_EQ("UUID(\"00112233-4455-6677-8899-aabbccddeeff\")",
              displayed(withUuid(mongo::newUUID).getField("u")));
    EXPECT_EQ("JUUID(\"77665544-3322-1100-ffee-ddccbbaa9988\")",
              displayed(withUuid(mongo::bdtUUID).getField("u"), Utc, JavaLegacy));
}

TEST(BsonUtilsDisplayTests, showsRegexWithFlags)
{
    mongo::BSONObjBuilder builder;
    builder.appendRegex("re", "^a", "im");

    EXPECT_EQ("/^a/im", displayed(builder.obj().getField("re")));
}

TEST(BsonUtilsDisplayTests, showsNestedDocumentsAsJson)
{
    mongo::BSONObj const obj = BSON("doc" << BSON("a" << 1));

    std::string const text = displayed(obj.getField("doc"));

    EXPECT_NE(std::string::npos, text.find("\"a\" : 1"));
}

// ---- Type helpers ----

TEST(BsonUtilsTypeTests, recognisesArraysAndDocuments)
{
    mongo::BSONObj const obj = BSON("a" << BSON_ARRAY(1) << "d" << BSON("x" << 1) << "s" << "text");

    EXPECT_TRUE(BsonUtils::isArray(obj.getField("a")));
    EXPECT_FALSE(BsonUtils::isArray(obj.getField("d")));
    EXPECT_TRUE(BsonUtils::isDocument(obj.getField("a")));
    EXPECT_TRUE(BsonUtils::isDocument(obj.getField("d")));
    EXPECT_FALSE(BsonUtils::isDocument(obj.getField("s")));
}

TEST(BsonUtilsTypeTests, simpleTypesAreEditableScalars)
{
    for (mongo::BSONType type : { mongo::NumberLong, mongo::NumberDouble, mongo::NumberDecimal,
                                  mongo::NumberInt, mongo::String, mongo::Bool, mongo::Date, mongo::jstOID })
        EXPECT_TRUE(BsonUtils::isSimpleType(type)) << type;

    for (mongo::BSONType type : { mongo::Object, mongo::Array, mongo::BinData, mongo::jstNULL,
                                  mongo::RegEx, mongo::Undefined, mongo::bsonTimestamp })
        EXPECT_FALSE(BsonUtils::isSimpleType(type)) << type;
}

TEST(BsonUtilsTypeTests, recognisesUuids)
{
    EXPECT_TRUE(BsonUtils::isUuidType(withUuid(mongo::newUUID).getField("u")));
    EXPECT_TRUE(BsonUtils::isUuidType(withUuid(mongo::bdtUUID).getField("u")));
    EXPECT_FALSE(BsonUtils::isUuidType(withUuid(mongo::BinDataGeneral).getField("u")));
    EXPECT_FALSE(BsonUtils::isUuidType(BSON("s" << "x").getField("s")));
    EXPECT_TRUE(BsonUtils::isUuidType(mongo::BinData, mongo::newUUID));
    EXPECT_FALSE(BsonUtils::isUuidType(mongo::String, mongo::newUUID));
}

TEST(BsonUtilsTypeTests, namesTypes)
{
    EXPECT_STREQ("Double", BsonUtils::BSONTypeToString(mongo::NumberDouble, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Int32", BsonUtils::BSONTypeToString(mongo::NumberInt, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Int64", BsonUtils::BSONTypeToString(mongo::NumberLong, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Decimal128", BsonUtils::BSONTypeToString(mongo::NumberDecimal, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("String", BsonUtils::BSONTypeToString(mongo::String, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Object", BsonUtils::BSONTypeToString(mongo::Object, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Array", BsonUtils::BSONTypeToString(mongo::Array, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("ObjectId", BsonUtils::BSONTypeToString(mongo::jstOID, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Boolean", BsonUtils::BSONTypeToString(mongo::Bool, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Date", BsonUtils::BSONTypeToString(mongo::Date, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Null", BsonUtils::BSONTypeToString(mongo::jstNULL, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Regular Expression", BsonUtils::BSONTypeToString(mongo::RegEx, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Timestamp", BsonUtils::BSONTypeToString(mongo::bsonTimestamp, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("Binary", BsonUtils::BSONTypeToString(mongo::BinData, mongo::BinDataGeneral, DefaultEncoding));
    EXPECT_STREQ("UUID", BsonUtils::BSONTypeToString(mongo::BinData, mongo::newUUID, JavaLegacy));
}

TEST(BsonUtilsTypeTests, namesLegacyUuidByEncoding)
{
    EXPECT_STREQ("Legacy UUID", BsonUtils::BSONTypeToString(mongo::BinData, mongo::bdtUUID, DefaultEncoding));
    EXPECT_STREQ("Java UUID (Legacy)", BsonUtils::BSONTypeToString(mongo::BinData, mongo::bdtUUID, JavaLegacy));
    EXPECT_STREQ(".NET UUID (Legacy)", BsonUtils::BSONTypeToString(mongo::BinData, mongo::bdtUUID, CSharpLegacy));
    EXPECT_STREQ("Python UUID (Legacy)", BsonUtils::BSONTypeToString(mongo::BinData, mongo::bdtUUID, PythonLegacy));
}

// ---- Field access ----

TEST(BsonUtilsFieldTests, readsTypedFields)
{
    mongo::BSONObj const obj = BSON("s" << "text" << "b" << true << "i" << 7 << "d" << 2.5 << "l" << 5LL
                                        << "doc" << BSON("x" << 1));

    EXPECT_EQ("text", BsonUtils::getField<mongo::String>(obj, "s"));
    EXPECT_TRUE(BsonUtils::getField<mongo::Bool>(obj, "b"));
    EXPECT_EQ(7, BsonUtils::getField<mongo::NumberInt>(obj, "i"));
    EXPECT_DOUBLE_EQ(2.5, BsonUtils::getField<mongo::NumberDouble>(obj, "d"));
    EXPECT_EQ(5, BsonUtils::getField<mongo::NumberLong>(obj, "l"));
    EXPECT_EQ(1, BsonUtils::getField<mongo::Object>(obj, "doc").getField("x").Int());
}

TEST(BsonUtilsFieldTests, missingFieldsGiveDefaults)
{
    mongo::BSONObj const obj;

    EXPECT_EQ("", BsonUtils::getField<mongo::String>(obj, "s"));
    EXPECT_FALSE(BsonUtils::getField<mongo::Bool>(obj, "b"));
    EXPECT_EQ(0, BsonUtils::getField<mongo::NumberInt>(obj, "i"));
    EXPECT_EQ(0, BsonUtils::getField<mongo::NumberLong>(obj, "l"));
    EXPECT_TRUE(BsonUtils::getField<mongo::Object>(obj, "doc").isEmpty());
}

TEST(BsonUtilsFieldTests, indexOfAndElementsCount)
{
    mongo::BSONObj const obj = BSON("a" << 1 << "b" << 2 << "c" << 3);

    EXPECT_EQ(3, BsonUtils::elementsCount(obj));
    EXPECT_EQ(0, BsonUtils::elementsCount(mongo::BSONObj()));
    EXPECT_EQ(std::string("b"), BsonUtils::indexOf(obj, 1).fieldName());
    EXPECT_EQ(std::string("c"), BsonUtils::indexOf(obj, 2).fieldName());
    EXPECT_TRUE(BsonUtils::indexOf(obj, 3).eoo());
    EXPECT_TRUE(BsonUtils::indexOf(obj, -1).eoo());
}

TEST(BsonUtilsFieldTests, reformatDoubleStringAddsDecimalForWholeNumbers)
{
    EXPECT_EQ("5.0", BsonUtils::reformatDoubleString("5", 5.0));
    EXPECT_EQ("-3.0", BsonUtils::reformatDoubleString("-3", -3.0));
    EXPECT_EQ("2.5", BsonUtils::reformatDoubleString("2.5", 2.5));
    EXPECT_EQ("1e-07", BsonUtils::reformatDoubleString("1e-07", 1e-7));
    EXPECT_EQ("1000000000000000.0", BsonUtils::reformatDoubleString("1e+15", 1e15));
}
