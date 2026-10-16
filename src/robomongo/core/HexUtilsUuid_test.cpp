#include <gtest/gtest.h>

#include <memory>

#include <mongo/bson/bsonobjbuilder.h>

#include "robomongo/core/HexUtils.h"

using namespace Robomongo;

namespace
{
    // 00112233-4455-6677-8899-aabbccddeeff
    const std::string Hex = "00112233445566778899aabbccddeeff";

    mongo::BSONObj binary(mongo::BinDataType type, const std::string &hex)
    {
        int length = 0;
        std::unique_ptr<const char[]> const bytes(HexUtils::fromHex(hex, &length));
        mongo::BSONObjBuilder builder;
        builder.appendBinData("u", length, type, bytes.get());
        return builder.obj();
    }
}

TEST(HexUtilsUuidTests, recognisesHexStrings)
{
    EXPECT_TRUE(HexUtils::isHexString("0123456789abcdefABCDEF"));
    EXPECT_TRUE(HexUtils::isHexString(""));
    EXPECT_FALSE(HexUtils::isHexString("0g"));
    EXPECT_FALSE(HexUtils::isHexString("00-11"));
    EXPECT_FALSE(HexUtils::isHexString(" 0"));
}

TEST(HexUtilsUuidTests, convertsBytesToLowerHex)
{
    char const raw[] = { '\x00', '\x0f', '\x10', '\xab', '\xff' };

    EXPECT_EQ("000f10abff", HexUtils::toStdHexLower(raw, sizeof(raw)));
    EXPECT_EQ("", HexUtils::toStdHexLower(raw, 0));
}

TEST(HexUtilsUuidTests, convertsHexToBytes)
{
    int length = 0;
    std::unique_ptr<const char[]> const bytes(HexUtils::fromHex("00ff10Ab", &length));

    ASSERT_EQ(4, length);
    EXPECT_EQ('\x00', bytes[0]);
    EXPECT_EQ('\xff', bytes[1]);
    EXPECT_EQ('\x10', bytes[2]);
    EXPECT_EQ('\xab', bytes[3]);
}

TEST(HexUtilsUuidTests, rejectsOddLengthHex)
{
    int length = -1;

    EXPECT_EQ(nullptr, HexUtils::fromHex("abc", &length));
}

TEST(HexUtilsUuidTests, hexAndBytesRoundTrip)
{
    int length = 0;
    std::unique_ptr<const char[]> const bytes(HexUtils::fromHex(Hex, &length));

    EXPECT_EQ(Hex, HexUtils::toStdHexLower(bytes.get(), length));
}

TEST(HexUtilsUuidTests, formatsStandardUuid)
{
    EXPECT_EQ("00112233-4455-6677-8899-aabbccddeeff", HexUtils::hexToUuid(Hex));
    EXPECT_EQ("00112233-4455-6677-8899-aabbccddeeff", HexUtils::hexToUuid(Hex, DefaultEncoding));
    EXPECT_EQ("00112233-4455-6677-8899-aabbccddeeff", HexUtils::hexToPythonUuid(Hex));
}

TEST(HexUtilsUuidTests, formatsLegacyUuidByteOrders)
{
    // .NET: first three groups little-endian
    EXPECT_EQ("33221100-5544-7766-8899-aabbccddeeff", HexUtils::hexToCSharpUuid(Hex));
    EXPECT_EQ("33221100-5544-7766-8899-aabbccddeeff", HexUtils::hexToUuid(Hex, CSharpLegacy));
    // Java: each 8-byte half reversed
    EXPECT_EQ("77665544-3322-1100-ffee-ddccbbaa9988", HexUtils::hexToJavaUuid(Hex));
    EXPECT_EQ("77665544-3322-1100-ffee-ddccbbaa9988", HexUtils::hexToUuid(Hex, JavaLegacy));
}

TEST(HexUtilsUuidTests, parsesUuidTextToHex)
{
    EXPECT_EQ(Hex, HexUtils::uuidToHex("00112233-4455-6677-8899-aabbccddeeff"));
    EXPECT_EQ(Hex, HexUtils::uuidToHex("{00112233-4455-6677-8899-aabbccddeeff}"));
    EXPECT_EQ(Hex, HexUtils::uuidToHex(Hex));
    EXPECT_EQ(Hex, HexUtils::pythonUuidToHex("00112233-4455-6677-8899-aabbccddeeff"));
}

TEST(HexUtilsUuidTests, rejectsUuidTextOfWrongLength)
{
    for (UUIDEncoding encoding : { DefaultEncoding, JavaLegacy, CSharpLegacy, PythonLegacy }) {
        EXPECT_EQ("", HexUtils::uuidToHex("00112233-4455", encoding));
        EXPECT_EQ("", HexUtils::uuidToHex("00112233-4455-6677-8899-aabbccddeeff00", encoding));
        EXPECT_EQ("", HexUtils::uuidToHex("", encoding));
    }
}

TEST(HexUtilsUuidTests, legacyConversionsAreInverses)
{
    for (UUIDEncoding encoding : { DefaultEncoding, JavaLegacy, CSharpLegacy, PythonLegacy }) {
        std::string const text = HexUtils::hexToUuid(Hex, encoding);
        EXPECT_EQ(Hex, HexUtils::uuidToHex(text, encoding)) << encoding;
    }
}

TEST(HexUtilsUuidTests, formatsStandardUuidElementRegardlessOfEncoding)
{
    mongo::BSONObj const obj = binary(mongo::newUUID, Hex);

    for (UUIDEncoding encoding : { DefaultEncoding, JavaLegacy, CSharpLegacy, PythonLegacy })
        EXPECT_EQ("UUID(\"00112233-4455-6677-8899-aabbccddeeff\")",
                  HexUtils::formatUuid(obj.getField("u"), encoding));
}

TEST(HexUtilsUuidTests, formatsLegacyUuidElementWithEncodingConstructor)
{
    mongo::BSONObj const keep = binary(mongo::bdtUUID, Hex);

    EXPECT_EQ("LUUID(\"00112233-4455-6677-8899-aabbccddeeff\")",
              HexUtils::formatUuid(keep.getField("u"), DefaultEncoding));
    EXPECT_EQ("JUUID(\"77665544-3322-1100-ffee-ddccbbaa9988\")",
              HexUtils::formatUuid(keep.getField("u"), JavaLegacy));
    EXPECT_EQ("NUUID(\"33221100-5544-7766-8899-aabbccddeeff\")",
              HexUtils::formatUuid(keep.getField("u"), CSharpLegacy));
    EXPECT_EQ("PYUUID(\"00112233-4455-6677-8899-aabbccddeeff\")",
              HexUtils::formatUuid(keep.getField("u"), PythonLegacy));
}

TEST(HexUtilsUuidTests, formatUuidRejectsNonUuidBinary)
{
    mongo::BSONObj const obj = binary(mongo::BinDataGeneral, Hex);

    EXPECT_THROW(HexUtils::formatUuid(obj.getField("u"), DefaultEncoding), std::invalid_argument);
}
