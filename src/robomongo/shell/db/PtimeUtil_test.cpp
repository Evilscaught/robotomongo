#include <gtest/gtest.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "robomongo/shell/db/ptimeutil.h"

using boost::posix_time::ptime;

namespace
{
    long long millisSinceEpoch(const ptime &time)
    {
        ptime const epoch(boost::gregorian::date(1970, 1, 1));
        return (time - epoch).total_milliseconds();
    }

    long long parseMillis(const std::string &text, bool *ok = nullptr)
    {
        bool success = false;
        ptime const time = miutil::ptimeFromIsoString(text, success);
        if (ok)
            *ok = success;
        EXPECT_TRUE(success || ok) << text;
        return success ? millisSinceEpoch(time) : 0;
    }

    ptime fromMillis(long long ms)
    {
        return ptime(boost::gregorian::date(1970, 1, 1)) + boost::posix_time::milliseconds(ms);
    }
}

TEST(PtimeUtilTests, parsesUtcWithMilliseconds)
{
    EXPECT_EQ(1700000000123LL, parseMillis("2023-11-14T22:13:20.123Z"));
}

TEST(PtimeUtilTests, parsesUtcWithoutMilliseconds)
{
    EXPECT_EQ(1700000000000LL, parseMillis("2023-11-14T22:13:20Z"));
}

TEST(PtimeUtilTests, timeWithoutZoneIsUtc)
{
    EXPECT_EQ(1700000000123LL, parseMillis("2023-11-14T22:13:20.123"));
}

TEST(PtimeUtilTests, acceptsSpaceAsDateTimeSeparator)
{
    EXPECT_EQ(1700000000123LL, parseMillis("2023-11-14 22:13:20.123Z"));
}

TEST(PtimeUtilTests, appliesColonOffsets)
{
    EXPECT_EQ(1700000000123LL, parseMillis("2023-11-15T00:13:20.123+02:00"));
    EXPECT_EQ(1700000000123LL, parseMillis("2023-11-14T15:13:20.123-07:00"));
    EXPECT_EQ(1700000000123LL, parseMillis("2023-11-15T03:43:20.123+05:30"));
}

TEST(PtimeUtilTests, ignoresLeadingWhitespace)
{
    EXPECT_EQ(1700000000123LL, parseMillis("   2023-11-14T22:13:20.123Z"));
}

TEST(PtimeUtilTests, parsesDatesAcrossCenturies)
{
    EXPECT_EQ(-2208988800000LL, parseMillis("1900-01-01T00:00:00.000Z"));
    EXPECT_EQ(-86400000LL, parseMillis("1969-12-31T00:00:00.000Z"));
    EXPECT_EQ(1709208000000LL, parseMillis("2024-02-29T12:00:00.000Z"));
    EXPECT_EQ(253402300799999LL, parseMillis("9999-12-31T23:59:59.999Z"));
}

TEST(PtimeUtilTests, parsesNamedTimes)
{
    bool ok = false;

    EXPECT_EQ(0, millisSinceEpoch(miutil::ptimeFromIsoString("epoch", ok)));
    EXPECT_TRUE(ok);
    EXPECT_TRUE(miutil::ptimeFromIsoString("infinity", ok).is_pos_infinity());
    EXPECT_TRUE(miutil::ptimeFromIsoString("-infinity", ok).is_neg_infinity());

    ptime const today = miutil::ptimeFromIsoString("today", ok);
    ptime const tomorrow = miutil::ptimeFromIsoString("tomorrow", ok);
    ptime const yesterday = miutil::ptimeFromIsoString("yesterday", ok);
    EXPECT_EQ(boost::posix_time::hours(0), today.time_of_day());
    EXPECT_EQ(boost::posix_time::hours(24), tomorrow - today);
    EXPECT_EQ(boost::posix_time::hours(24), today - yesterday);
}

TEST(PtimeUtilTests, parsesRfc1123Dates)
{
    EXPECT_EQ(1175421064000LL, parseMillis("Sun, 01 Apr 2007 09:51:04 GMT"));
}

TEST(PtimeUtilTests, rejectsInvalidDates)
{
    bool ok = true;

    miutil::ptimeFromIsoString("2023-13-01T00:00:00Z", ok);
    EXPECT_FALSE(ok);

    ok = true;
    miutil::ptimeFromIsoString("2023-02-30T00:00:00Z", ok);
    EXPECT_FALSE(ok);
}

TEST(PtimeUtilTests, formatsUtcIsoString)
{
    ptime const time = fromMillis(1700000000123LL);

    EXPECT_EQ("2023-11-14 22:13:20.123Z", miutil::isotimeString(time));
    EXPECT_EQ("2023-11-14T22:13:20.123Z", miutil::isotimeString(time, true));
}

TEST(PtimeUtilTests, formatsMillisecondsWithLeadingZeros)
{
    EXPECT_EQ("1970-01-01T00:00:00.007Z", miutil::isotimeString(fromMillis(7), true));
    EXPECT_EQ("1969-12-31T23:59:59.999Z", miutil::isotimeString(fromMillis(-1), true));
}

TEST(PtimeUtilTests, specialTimesFormatAsEmpty)
{
    EXPECT_EQ("", miutil::isotimeString(ptime(boost::posix_time::not_a_date_time)));
}

TEST(PtimeUtilTests, utcFormatRoundTrips)
{
    for (long long ms : { 0LL, 1700000000123LL, -86400000LL, -2208988800000LL, 253402300799999LL }) {
        std::string const text = miutil::isotimeString(fromMillis(ms), true);
        EXPECT_EQ(ms, parseMillis(text)) << text;
    }
}

TEST(PtimeUtilTests, localFormatRoundTripsToSameInstant)
{
    // Local format carries an explicit +HH:MM offset, so parsing it gives back the same instant
    for (long long ms : { 0LL, 1700000000123LL, 1720000000456LL }) {
        std::string const text = miutil::isotimeString(fromMillis(ms), true, true);
        EXPECT_EQ(ms, parseMillis(text)) << text;
        EXPECT_NE('Z', text.back()) << text;
    }
}

TEST(PtimeUtilTests, formatsRfc1123Date)
{
    EXPECT_EQ("Sun, 01 Apr 2007 09:51:04 GMT", miutil::rfc1123date(fromMillis(1175421064000LL)));
}

TEST(PtimeUtilTests, rfc1123RoundTrips)
{
    ptime const time = fromMillis(1700000000000LL);

    EXPECT_EQ(time, miutil::rfc1123date(miutil::rfc1123date(time)));
}
