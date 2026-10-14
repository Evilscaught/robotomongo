#include <gtest/gtest.h>

#include "robomongo/core/settings/ResultsRefreshSettings.h"

using namespace Robomongo;

TEST(ResultsRefreshSettingsTests, autoRefreshIsEnabledWhenConfigHasNoValue)
{
    QVariantMap const map;
    EXPECT_TRUE(ResultsRefreshSettings::loadAutoRefresh(map));
}

TEST(ResultsRefreshSettingsTests, loadsDisabledAutoRefresh)
{
    QVariantMap map;
    map.insert("autoRefreshResults", false);
    EXPECT_FALSE(ResultsRefreshSettings::loadAutoRefresh(map));
}

TEST(ResultsRefreshSettingsTests, loadsEnabledAutoRefresh)
{
    QVariantMap map;
    map.insert("autoRefreshResults", true);
    EXPECT_TRUE(ResultsRefreshSettings::loadAutoRefresh(map));
}

TEST(ResultsRefreshSettingsTests, savesUnderConfigKey)
{
    QVariantMap map;
    ResultsRefreshSettings::saveAutoRefresh(map, false);
    ASSERT_TRUE(map.contains("autoRefreshResults"));
    EXPECT_FALSE(map.value("autoRefreshResults").toBool());

    ResultsRefreshSettings::saveAutoRefresh(map, true);
    EXPECT_TRUE(map.value("autoRefreshResults").toBool());
}

TEST(ResultsRefreshSettingsTests, roundTripsBothValues)
{
    for (bool const value : { true, false }) {
        QVariantMap map;
        ResultsRefreshSettings::saveAutoRefresh(map, value);
        EXPECT_EQ(value, ResultsRefreshSettings::loadAutoRefresh(map));
    }
}
