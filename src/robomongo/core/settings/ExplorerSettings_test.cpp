#include <gtest/gtest.h>

#include <parser.h>
#include <serializer.h>

#include "robomongo/core/settings/ExplorerSettings.h"

using namespace Robomongo;

TEST(ExplorerSettingsTests, autoRevealIsEnabledWhenConfigHasNoValue)
{
    QVariantMap const map;
    EXPECT_TRUE(ExplorerSettings::loadAutoReveal(map));
}

TEST(ExplorerSettingsTests, loadsDisabledAutoReveal)
{
    QVariantMap map;
    map.insert("explorerAutoReveal", false);
    EXPECT_FALSE(ExplorerSettings::loadAutoReveal(map));
}

TEST(ExplorerSettingsTests, loadsEnabledAutoReveal)
{
    QVariantMap map;
    map.insert("explorerAutoReveal", true);
    EXPECT_TRUE(ExplorerSettings::loadAutoReveal(map));
}

TEST(ExplorerSettingsTests, savesUnderConfigKey)
{
    QVariantMap map;
    ExplorerSettings::saveAutoReveal(map, false);
    ASSERT_TRUE(map.contains("explorerAutoReveal"));
    EXPECT_FALSE(map.value("explorerAutoReveal").toBool());

    ExplorerSettings::saveAutoReveal(map, true);
    EXPECT_TRUE(map.value("explorerAutoReveal").toBool());
}

TEST(ExplorerSettingsTests, roundTripsBothValues)
{
    for (bool const value : { true, false }) {
        QVariantMap map;
        ExplorerSettings::saveAutoReveal(map, value);
        EXPECT_EQ(value, ExplorerSettings::loadAutoReveal(map));
    }
}

TEST(ExplorerSettingsTests, survivesConfigFileJsonRoundTrip)
{
    // The config file is written and read with QJson
    QVariantMap map;
    map.insert("batchSize", 50);
    ExplorerSettings::saveAutoReveal(map, false);

    bool serialized = false;
    QJson::Serializer serializer;
    serializer.setIndentMode(QJson::IndentFull);
    QByteArray const json = serializer.serialize(map, &serialized);
    ASSERT_TRUE(serialized);
    EXPECT_TRUE(json.contains("\"explorerAutoReveal\""));

    bool parsed = false;
    QJson::Parser parser;
    QVariantMap const loaded = parser.parse(json, &parsed).toMap();
    ASSERT_TRUE(parsed);
    EXPECT_FALSE(ExplorerSettings::loadAutoReveal(loaded));
    EXPECT_EQ(50, loaded.value("batchSize").toInt());
}
