#include <gtest/gtest.h>

#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo/shell/bson/json.h"

using mongo::Robomongo::fromjson;

namespace
{
    mongo::BSONElement field(const mongo::BSONObj &obj, const char *name)
    {
        mongo::BSONElement const element = obj.getField(name);
        EXPECT_FALSE(element.eoo()) << name << " missing in " << obj.toString();
        return element;
    }

    bool parses(const std::string &json)
    {
        try {
            fromjson(json);
            return true;
        } catch (const std::exception &) {
            return false;
        }
    }

    std::string binHex(const mongo::BSONElement &element)
    {
        int length = 0;
        const char *data = element.binData(length);
        static const char digits[] = "0123456789abcdef";
        std::string hex;
        for (int i = 0; i < length; ++i) {
            hex += digits[(static_cast<unsigned char>(data[i]) >> 4) & 0xf];
            hex += digits[static_cast<unsigned char>(data[i]) & 0xf];
        }
        return hex;
    }
}

// ---- Plain JSON ----

TEST(JsonParserTests, parsesEmptyDocument)
{
    EXPECT_TRUE(fromjson("{}").isEmpty());
    EXPECT_TRUE(fromjson("  {  }  ").isEmpty());
}

TEST(JsonParserTests, parsesStringsBoolsAndNull)
{
    mongo::BSONObj const obj = fromjson(R"({ "s": "text", "t": true, "f": false, "n": null })");

    EXPECT_EQ("text", field(obj, "s").str());
    EXPECT_TRUE(field(obj, "t").Bool());
    EXPECT_FALSE(field(obj, "f").Bool());
    EXPECT_EQ(mongo::jstNULL, field(obj, "n").type());
}

TEST(JsonParserTests, keepsFieldOrder)
{
    EXPECT_EQ("{ z: 1, a: 2, m: 3 }", fromjson("{ z: 1, a: 2, m: 3 }").toString());
}

TEST(JsonParserTests, acceptsUnquotedAndSingleQuotedFieldNames)
{
    mongo::BSONObj const obj = fromjson(R"({ unquoted: 1, 'single': 'value', "double": 2 })");

    EXPECT_EQ(1, field(obj, "unquoted").Int());
    EXPECT_EQ("value", field(obj, "single").str());
    EXPECT_EQ(2, field(obj, "double").Int());
}

TEST(JsonParserTests, decodesStringEscapes)
{
    mongo::BSONObj const obj = fromjson(R"({ "s": "quote \" backslash \\ slash \/ tab \t newline \n" })");

    EXPECT_EQ("quote \" backslash \\ slash / tab \t newline \n", field(obj, "s").str());
}

TEST(JsonParserTests, decodesUnicodeEscapesToUtf8)
{
    mongo::BSONObj const obj = fromjson(R"({ "s": "café €" })");

    EXPECT_EQ("café €", field(obj, "s").str());
}

TEST(JsonParserTests, keepsRawUtf8)
{
    mongo::BSONObj const obj = fromjson(R"({ "名前": "Ünïcødé 🚀" })");

    EXPECT_EQ("Ünïcødé 🚀", field(obj, "名前").str());
}

TEST(JsonParserTests, parsesNestedDocumentsAndArrays)
{
    mongo::BSONObj const obj = fromjson(R"({ "a": { "b": [1, [2, 3], { "c": "d" }] }, "e": [] })");

    mongo::BSONObj const array = field(obj, "a").Obj().getField("b").Obj();
    EXPECT_EQ(1, array.getField("0").Int());
    EXPECT_EQ(mongo::Array, array.getField("1").type());
    EXPECT_EQ("d", array.getField("2").Obj().getField("c").str());
    EXPECT_EQ(mongo::Array, field(obj, "e").type());
    EXPECT_TRUE(field(obj, "e").Obj().isEmpty());
}

TEST(JsonParserTests, rejectsMalformedJson)
{
    EXPECT_FALSE(parses("{"));
    EXPECT_FALSE(parses("{ a: }"));
    EXPECT_FALSE(parses("{ a: 1,, b: 2 }"));
    EXPECT_FALSE(parses("{ \"a\": \"unterminated }"));
    EXPECT_FALSE(parses("{ a: 1 "));
    EXPECT_FALSE(parses("{ a: tru }"));
}

TEST(JsonParserTests, emptyInputIsEmptyDocument)
{
    EXPECT_TRUE(fromjson("").isEmpty());
}

TEST(JsonParserTests, topLevelArrayBecomesIndexedDocument)
{
    // Used by the insert dialog to accept several documents; see isArray()
    EXPECT_EQ("{ 0: 1, 1: \"two\" }", fromjson("[1, \"two\"]").toString());
}

TEST(JsonParserTests, parsingStopsAfterFirstDocument)
{
    // Callers use the reported length to continue with the next document
    int length = 0;
    std::string const input = "{ a: 1 } { b: 2 }";

    mongo::BSONObj const first = fromjson(input.c_str(), &length);
    mongo::BSONObj const second = fromjson(input.c_str() + length, nullptr);

    EXPECT_EQ("{ a: 1 }", first.toString());
    EXPECT_EQ("{ b: 2 }", second.toString());
}

TEST(JsonParserTests, reportsLengthOfParsedJson)
{
    int length = 0;

    fromjson("{ a: 1 }   ", &length);

    EXPECT_EQ(8, length);
}

// ---- Numbers ----

TEST(JsonParserTests, smallIntegersBecomeInt32)
{
    mongo::BSONObj const obj = fromjson("{ a: 0, b: -5, c: 2147483647, d: -2147483648 }");

    for (const char *name : { "a", "b", "c", "d" })
        EXPECT_EQ(mongo::NumberInt, field(obj, name).type()) << name;
    EXPECT_EQ(2147483647, field(obj, "c").Int());
    EXPECT_EQ(-2147483648LL, field(obj, "d").Int());
}

TEST(JsonParserTests, integersBeyondInt32BecomeInt64)
{
    mongo::BSONObj const obj = fromjson("{ a: 2147483648, b: -2147483649, c: 9223372036854775807 }");

    EXPECT_EQ(mongo::NumberLong, field(obj, "a").type());
    EXPECT_EQ(2147483648LL, field(obj, "a").Long());
    EXPECT_EQ(-2147483649LL, field(obj, "b").Long());
    EXPECT_EQ(9223372036854775807LL, field(obj, "c").Long());
}

TEST(JsonParserTests, decimalsAndExponentsBecomeDouble)
{
    mongo::BSONObj const obj = fromjson("{ a: 2.5, b: 1e3, c: -0.001, d: 5.0, e: 1E-2 }");

    for (const char *name : { "a", "b", "c", "d", "e" })
        EXPECT_EQ(mongo::NumberDouble, field(obj, name).type()) << name;
    EXPECT_DOUBLE_EQ(2.5, field(obj, "a").Double());
    EXPECT_DOUBLE_EQ(1000, field(obj, "b").Double());
    EXPECT_DOUBLE_EQ(5.0, field(obj, "d").Double());
}

TEST(JsonParserTests, integersTooBigForInt64BecomeDouble)
{
    mongo::BSONObj const obj = fromjson("{ a: 92233720368547758070 }");

    EXPECT_EQ(mongo::NumberDouble, field(obj, "a").type());
}

TEST(JsonParserTests, parsesSpecialDoubles)
{
    mongo::BSONObj const obj = fromjson("{ nan: NaN, inf: Infinity, ninf: -Infinity }");

    EXPECT_TRUE(std::isnan(field(obj, "nan").Double()));
    EXPECT_TRUE(std::isinf(field(obj, "inf").Double()));
    EXPECT_GT(field(obj, "inf").Double(), 0);
    EXPECT_LT(field(obj, "ninf").Double(), 0);
}

TEST(JsonParserTests, parsesNumberIntLongAndDecimalConstructors)
{
    mongo::BSONObj const obj = fromjson(
        R"({ i: NumberInt(7), l: NumberLong(5), ls: NumberLong(9007199254740993), d: NumberDecimal("1.10") })");

    EXPECT_EQ(mongo::NumberInt, field(obj, "i").type());
    EXPECT_EQ(7, field(obj, "i").Int());
    EXPECT_EQ(mongo::NumberLong, field(obj, "l").type());
    EXPECT_EQ(5, field(obj, "l").Long());
    EXPECT_EQ(9007199254740993LL, field(obj, "ls").Long());
    EXPECT_EQ(mongo::NumberDecimal, field(obj, "d").type());
    EXPECT_EQ("1.10", field(obj, "d").numberDecimal().toString());
}

TEST(JsonParserTests, parsesExtendedJsonNumbers)
{
    mongo::BSONObj const obj = fromjson(
        R"({ l: { "$numberLong": "5" }, d: { "$numberDecimal": "0.1" } })");

    EXPECT_EQ(mongo::NumberLong, field(obj, "l").type());
    EXPECT_EQ(5, field(obj, "l").Long());
    EXPECT_EQ(mongo::NumberDecimal, field(obj, "d").type());
    EXPECT_EQ("0.1", field(obj, "d").numberDecimal().toString());
}

// ---- ObjectId ----

TEST(JsonParserTests, parsesObjectIdForms)
{
    mongo::BSONObj const obj = fromjson(
        R"({ a: ObjectId("5f1d7a2b3c4d5e6f70819203"), b: { "$oid": "5f1d7a2b3c4d5e6f70819203" } })");

    EXPECT_EQ(mongo::jstOID, field(obj, "a").type());
    EXPECT_EQ("5f1d7a2b3c4d5e6f70819203", field(obj, "a").OID().toString());
    EXPECT_EQ(field(obj, "a").OID(), field(obj, "b").OID());
}

TEST(JsonParserTests, rejectsInvalidObjectId)
{
    EXPECT_FALSE(parses(R"({ a: ObjectId("123") })"));
    EXPECT_FALSE(parses(R"({ a: ObjectId("zzzzzzzzzzzzzzzzzzzzzzzz") })"));
    EXPECT_FALSE(parses(R"({ a: { "$oid": "5f1d7a2b3c4d5e6f7081920" } })"));
}

// ---- Dates ----

TEST(JsonParserTests, parsesIsoDateInUtc)
{
    mongo::BSONObj const obj = fromjson(R"({ d: ISODate("2023-11-14T22:13:20.123Z") })");

    EXPECT_EQ(mongo::Date, field(obj, "d").type());
    EXPECT_EQ(1700000000123LL, field(obj, "d").date().toMillisSinceEpoch());
}

TEST(JsonParserTests, parsesIsoDateWithColonOffsets)
{
    // The format Robo 3T itself writes when showing dates in local time
    mongo::BSONObj const obj = fromjson(
        R"({ plus: ISODate("2023-11-15T00:13:20.123+02:00"), minus: ISODate("2023-11-14T15:13:20.123-07:00") })");

    EXPECT_EQ(1700000000123LL, field(obj, "plus").date().toMillisSinceEpoch());
    EXPECT_EQ(1700000000123LL, field(obj, "minus").date().toMillisSinceEpoch());
}

TEST(JsonParserTests, parsesIsoDateWithoutZoneAsUtc)
{
    mongo::BSONObj const obj = fromjson(R"({ d: ISODate("2023-11-14T22:13:20.123") })");

    EXPECT_EQ(1700000000123LL, field(obj, "d").date().toMillisSinceEpoch());
}

TEST(JsonParserTests, parsesIsoDateWithoutMilliseconds)
{
    mongo::BSONObj const obj = fromjson(R"({ d: ISODate("2023-11-14T22:13:20Z") })");

    EXPECT_EQ(1700000000000LL, field(obj, "d").date().toMillisSinceEpoch());
}

TEST(JsonParserTests, parsesIsoDateEdgeYears)
{
    mongo::BSONObj const obj = fromjson(
        R"({ leap: ISODate("2024-02-29T12:00:00Z"), y1900: ISODate("1900-01-01T00:00:00Z"),
             y9999: ISODate("9999-12-31T23:59:59.999Z") })");

    EXPECT_EQ(1709208000000LL, field(obj, "leap").date().toMillisSinceEpoch());
    EXPECT_EQ(-2208988800000LL, field(obj, "y1900").date().toMillisSinceEpoch());
    EXPECT_EQ(253402300799999LL, field(obj, "y9999").date().toMillisSinceEpoch());
}

TEST(JsonParserTests, rejectsInvalidIsoDate)
{
    EXPECT_FALSE(parses(R"({ d: ISODate("not a date") })"));
    EXPECT_FALSE(parses(R"({ d: ISODate("2023-13-01T00:00:00Z") })"));
}

TEST(JsonParserTests, parsesDateConstructorsFromMilliseconds)
{
    mongo::BSONObj const obj = fromjson(
        "{ a: Date(1700000000123), b: new Date(1700000000123), c: Date(-86400000) }");

    EXPECT_EQ(1700000000123LL, field(obj, "a").date().toMillisSinceEpoch());
    EXPECT_EQ(1700000000123LL, field(obj, "b").date().toMillisSinceEpoch());
    EXPECT_EQ(-86400000LL, field(obj, "c").date().toMillisSinceEpoch());
}

TEST(JsonParserTests, parsesExtendedJsonDates)
{
    mongo::BSONObj const obj = fromjson(
        R"({ ms: { "$date": 1700000000123 }, iso: { "$date": "2023-11-14T22:13:20.123Z" },
             long: { "$date": { "$numberLong": "-86400000" } } })");

    EXPECT_EQ(1700000000123LL, field(obj, "ms").date().toMillisSinceEpoch());
    EXPECT_EQ(1700000000123LL, field(obj, "iso").date().toMillisSinceEpoch());
    EXPECT_EQ(-86400000LL, field(obj, "long").date().toMillisSinceEpoch());
}

// ---- Timestamps ----

TEST(JsonParserTests, parsesTimestampForms)
{
    mongo::BSONObj const obj = fromjson(
        R"({ a: Timestamp(1700000000, 7), b: { "$timestamp": { "t": 1700000000, "i": 7 } } })");

    EXPECT_EQ(mongo::bsonTimestamp, field(obj, "a").type());
    EXPECT_EQ(1700000000u, field(obj, "a").timestamp().getSecs());
    EXPECT_EQ(7u, field(obj, "a").timestamp().getInc());
    EXPECT_EQ(field(obj, "a").timestamp(), field(obj, "b").timestamp());
}

// ---- Binary and UUIDs ----

TEST(JsonParserTests, parsesExtendedJsonBinary)
{
    mongo::BSONObj const obj = fromjson(R"({ b: { "$binary": "AQID+g==", "$type": "00" } })");

    EXPECT_EQ(mongo::BinData, field(obj, "b").type());
    EXPECT_EQ(mongo::BinDataGeneral, field(obj, "b").binDataType());
    EXPECT_EQ("010203fa", binHex(field(obj, "b")));
}

TEST(JsonParserTests, parsesStandardUuid)
{
    mongo::BSONObj const obj = fromjson(R"({ u: UUID("00112233-4455-6677-8899-aabbccddeeff") })");

    EXPECT_EQ(mongo::newUUID, field(obj, "u").binDataType());
    EXPECT_EQ("00112233445566778899aabbccddeeff", binHex(field(obj, "u")));
}

TEST(JsonParserTests, parsesLegacyUuidsWithDriverByteOrders)
{
    // Legacy (subtype 3) UUIDs were stored in driver-specific byte orders
    mongo::BSONObj const obj = fromjson(
        R"({ l: LUUID("00112233-4455-6677-8899-aabbccddeeff"),
             py: PYUUID("00112233-4455-6677-8899-aabbccddeeff"),
             cs: CSUUID("00112233-4455-6677-8899-aabbccddeeff"),
             n: NUUID("00112233-4455-6677-8899-aabbccddeeff"),
             j: JUUID("00112233-4455-6677-8899-aabbccddeeff") })");

    for (const char *name : { "l", "py", "cs", "n", "j" })
        EXPECT_EQ(mongo::bdtUUID, field(obj, name).binDataType()) << name;
    EXPECT_EQ("00112233445566778899aabbccddeeff", binHex(field(obj, "l")));
    EXPECT_EQ("00112233445566778899aabbccddeeff", binHex(field(obj, "py")));
    EXPECT_EQ("33221100554477668899aabbccddeeff", binHex(field(obj, "cs")));
    EXPECT_EQ(binHex(field(obj, "cs")), binHex(field(obj, "n")));
    EXPECT_EQ("7766554433221100ffeeddccbbaa9988", binHex(field(obj, "j")));
}

TEST(JsonParserTests, rejectsMalformedUuid)
{
    EXPECT_FALSE(parses(R"({ u: UUID("not-a-uuid") })"));
    EXPECT_FALSE(parses(R"({ u: UUID("00112233") })"));
}

// ---- Regex, special keys, DBRef ----

TEST(JsonParserTests, parsesRegexForms)
{
    mongo::BSONObj const obj = fromjson(
        R"({ a: /^ab+c$/i, b: { "$regex": "^ab+c$", "$options": "i" }, c: /plain/ })");

    EXPECT_EQ(mongo::RegEx, field(obj, "a").type());
    EXPECT_EQ(std::string("^ab+c$"), field(obj, "a").regex());
    EXPECT_EQ(std::string("i"), field(obj, "a").regexFlags());
    EXPECT_EQ(std::string("^ab+c$"), field(obj, "b").regex());
    EXPECT_EQ(std::string("i"), field(obj, "b").regexFlags());
    EXPECT_EQ(std::string(""), field(obj, "c").regexFlags());
}

TEST(JsonParserTests, parsesSupportedRegexOptions)
{
    mongo::BSONObj const obj = fromjson(R"({ a: /x/ims })");

    EXPECT_EQ(std::string("ims"), field(obj, "a").regexFlags());
}

TEST(JsonParserTests, parsesMinMaxKeyAndUndefined)
{
    mongo::BSONObj const obj = fromjson(
        R"({ min: { "$minKey": 1 }, max: { "$maxKey": 1 }, u: undefined, eu: { "$undefined": true } })");

    EXPECT_EQ(mongo::MinKey, field(obj, "min").type());
    EXPECT_EQ(mongo::MaxKey, field(obj, "max").type());
    EXPECT_EQ(mongo::Undefined, field(obj, "u").type());
    EXPECT_EQ(mongo::Undefined, field(obj, "eu").type());
}

TEST(JsonParserTests, parsesDbRef)
{
    mongo::BSONObj const obj = fromjson(
        R"({ r: DBRef("users", ObjectId("5f1d7a2b3c4d5e6f70819203")) })");

    mongo::BSONObj const ref = field(obj, "r").Obj();
    EXPECT_EQ("users", ref.getField("$ref").str());
    EXPECT_EQ("5f1d7a2b3c4d5e6f70819203", ref.getField("$id").OID().toString());
}

TEST(JsonParserTests, isArrayDetectsTopLevelArrays)
{
    EXPECT_TRUE(mongo::Robomongo::isArray("[1, 2]"));
    EXPECT_TRUE(mongo::Robomongo::isArray("  [ ]"));
    EXPECT_FALSE(mongo::Robomongo::isArray("{ a: [1] }"));
}

// ---- Round trip with the shell's own serializer ----

TEST(JsonParserTests, roundTripsStrictJsonOfCommonTypes)
{
    mongo::BSONObjBuilder builder;
    builder.append("_id", mongo::OID("5f1d7a2b3c4d5e6f70819203"));
    builder.append("name", "Ada");
    builder.append("age", 36);
    builder.append("big", 9007199254740993LL);
    builder.append("score", 2.5);
    builder.appendDate("at", mongo::Date_t::fromMillisSinceEpoch(1700000000123LL));
    builder.append("tags", BSON_ARRAY("a" << "b"));
    builder.append("price", mongo::Decimal128("19.99"));
    mongo::BSONObj const original = builder.obj();

    mongo::BSONObj const parsed = fromjson(original.jsonString(mongo::Strict, false));

    EXPECT_EQ(0, original.woCompare(parsed, mongo::BSONObj(), true)) << parsed.toString();
    EXPECT_EQ(mongo::NumberLong, field(parsed, "big").type());
    EXPECT_EQ(mongo::NumberDecimal, field(parsed, "price").type());
}
