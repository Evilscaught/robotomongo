#include <gtest/gtest.h>

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo/shell/bson/json.h"

// On MongoDB 6.0+ servers every document is converted between the legacy shell BSON and the
// modern driver's BSON through JSON text. These helpers are the exact expressions used by
// toModernBson() and toLegacyBson() in ModernMongoDriver.cpp (which are file-local).
namespace
{
    bsoncxx::document::value toModernBson(const mongo::BSONObj &obj)
    {
        return bsoncxx::from_json(obj.jsonString(mongo::Strict, false));
    }

    mongo::BSONObj toLegacyBson(const bsoncxx::document::view &doc)
    {
        return mongo::Robomongo::fromjson(bsoncxx::to_json(doc));
    }

    mongo::BSONObj roundTrip(const mongo::BSONObj &obj)
    {
        return toLegacyBson(toModernBson(obj).view());
    }

    // Same type and same value
    void expectSameElement(const mongo::BSONObj &original, const mongo::BSONObj &converted,
                           const char *field)
    {
        mongo::BSONElement const before = original.getField(field);
        mongo::BSONElement const after = converted.getField(field);
        ASSERT_FALSE(after.eoo()) << field << " missing";
        EXPECT_EQ(before.type(), after.type()) << field;
        EXPECT_EQ(0, before.woCompare(after, false)) << field << ": " << after.toString();
    }
}

TEST(ModernBsonConversionTests, keepsFieldOrder)
{
    mongo::BSONObj const original = BSON("z" << 1 << "a" << 2 << "m" << 3);

    mongo::BSONObj const converted = roundTrip(original);

    EXPECT_EQ(original.toString(), converted.toString());
}

TEST(ModernBsonConversionTests, keepsStringsIncludingUnicodeAndEscapes)
{
    mongo::BSONObj const original = BSON("s" << "héllo \"quoted\" back\\slash\nnew line\ttab 🚀"
                                             << "empty" << "");

    mongo::BSONObj const converted = roundTrip(original);

    expectSameElement(original, converted, "s");
    expectSameElement(original, converted, "empty");
}

TEST(ModernBsonConversionTests, keepsInt32AndFractionalDouble)
{
    mongo::BSONObj const original = BSON("i" << 42 << "neg" << -7 << "d" << 2.5 << "tiny" << 1e-9);

    mongo::BSONObj const converted = roundTrip(original);

    for (const char *field : { "i", "neg", "d", "tiny" })
        expectSameElement(original, converted, field);
}

TEST(ModernBsonConversionTests, keepsInt64OutsideInt32Range)
{
    mongo::BSONObj const original = BSON("big" << 9007199254740993LL << "neg" << -9007199254740993LL);

    mongo::BSONObj const converted = roundTrip(original);

    expectSameElement(original, converted, "big");
    expectSameElement(original, converted, "neg");
}

TEST(ModernBsonConversionTests, keepsDecimal128Exactly)
{
    mongo::BSONObj const original = BSON("price" << mongo::Decimal128("1.10")
                                                 << "huge" << mongo::Decimal128("12345678901234567890.123"));

    mongo::BSONObj const converted = roundTrip(original);

    expectSameElement(original, converted, "price");
    expectSameElement(original, converted, "huge");
    EXPECT_EQ("1.10", converted.getField("price").numberDecimal().toString());
}

TEST(ModernBsonConversionTests, keepsDatesToTheMillisecond)
{
    mongo::BSONObjBuilder builder;
    builder.appendDate("now", mongo::Date_t::fromMillisSinceEpoch(1700000000123LL));
    builder.appendDate("epoch", mongo::Date_t::fromMillisSinceEpoch(0));
    builder.appendDate("before1970", mongo::Date_t::fromMillisSinceEpoch(-86400000LL));
    mongo::BSONObj const original = builder.obj();

    mongo::BSONObj const converted = roundTrip(original);

    for (const char *field : { "now", "epoch", "before1970" }) {
        expectSameElement(original, converted, field);
        EXPECT_EQ(original.getField(field).date(), converted.getField(field).date()) << field;
    }
}

TEST(ModernBsonConversionTests, keepsObjectId)
{
    mongo::BSONObj const original = BSON("_id" << mongo::OID("5f1d7a2b3c4d5e6f70819203"));

    mongo::BSONObj const converted = roundTrip(original);

    expectSameElement(original, converted, "_id");
    EXPECT_EQ("5f1d7a2b3c4d5e6f70819203", converted.getField("_id").OID().toString());
}

TEST(ModernBsonConversionTests, keepsBinaryDataAndSubtype)
{
    unsigned char const bytes[] = { 0x00, 0x01, 0x7f, 0x80, 0xfe, 0xff };
    unsigned char const uuid[16] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
                                     0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
    mongo::BSONObjBuilder builder;
    builder.appendBinData("bin", sizeof(bytes), mongo::BinDataGeneral, bytes);
    builder.appendBinData("uuid", sizeof(uuid), mongo::newUUID, uuid);
    mongo::BSONObj const original = builder.obj();

    mongo::BSONObj const converted = roundTrip(original);

    expectSameElement(original, converted, "bin");
    expectSameElement(original, converted, "uuid");
    EXPECT_EQ(mongo::newUUID, converted.getField("uuid").binDataType());
}

TEST(ModernBsonConversionTests, keepsRegexWithOptions)
{
    mongo::BSONObjBuilder builder;
    builder.appendRegex("re", "^ab+c\\d$", "ims");
    mongo::BSONObj const original = builder.obj();

    mongo::BSONObj const converted = roundTrip(original);

    expectSameElement(original, converted, "re");
    EXPECT_EQ(std::string("^ab+c\\d$"), converted.getField("re").regex());
    EXPECT_EQ(std::string("ims"), converted.getField("re").regexFlags());
}

TEST(ModernBsonConversionTests, keepsTimestamp)
{
    mongo::BSONObj const original = BSON("ts" << mongo::Timestamp(1700000000, 7));

    mongo::BSONObj const converted = roundTrip(original);

    expectSameElement(original, converted, "ts");
}

TEST(ModernBsonConversionTests, keepsSpecialValues)
{
    mongo::BSONObjBuilder builder;
    builder.appendMinKey("min");
    builder.appendMaxKey("max");
    builder.appendNull("null");
    builder.append("yes", true);
    builder.append("no", false);
    mongo::BSONObj const original = builder.obj();

    mongo::BSONObj const converted = roundTrip(original);

    for (const char *field : { "min", "max", "null", "yes", "no" })
        expectSameElement(original, converted, field);
}

TEST(ModernBsonConversionTests, keepsNestedDocumentsAndArrays)
{
    mongo::BSONObj const original = BSON(
        "user" << BSON("name" << "Ada" << "tags" << BSON_ARRAY("a" << "b"))
               << "matrix" << BSON_ARRAY(BSON_ARRAY(1 << 2) << BSON_ARRAY(3 << 4))
               << "empty" << mongo::BSONObj()
               << "emptyArr" << mongo::BSONArray());

    mongo::BSONObj const converted = roundTrip(original);

    EXPECT_EQ(original.toString(), converted.toString());
    EXPECT_EQ(mongo::Object, converted.getField("empty").type());
    EXPECT_EQ(mongo::Array, converted.getField("emptyArr").type());
}

TEST(ModernBsonConversionTests, keepsUnusualFieldNames)
{
    mongo::BSONObj const original = BSON("with space" << 1 << "ünïcode" << 2 << "" << 3);

    mongo::BSONObj const converted = roundTrip(original);

    EXPECT_EQ(original.toString(), converted.toString());
}

TEST(ModernBsonConversionTests, writesInt64AsModernInt64)
{
    // Write direction: values typed by the user as NumberLong reach the server as int64
    auto const modern = toModernBson(BSON("n" << 5LL));

    EXPECT_EQ(bsoncxx::type::k_int64, modern.view()["n"].type());
}

TEST(ModernBsonConversionTests, writesDatesAsModernDates)
{
    mongo::BSONObjBuilder builder;
    builder.appendDate("at", mongo::Date_t::fromMillisSinceEpoch(1700000000123LL));

    auto const modern = toModernBson(builder.obj());

    ASSERT_EQ(bsoncxx::type::k_date, modern.view()["at"].type());
    EXPECT_EQ(1700000000123LL, modern.view()["at"].get_date().to_int64());
}
