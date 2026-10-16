#include <gtest/gtest.h>

#include <memory>

#include "robomongo/core/settings/ReplicaSetSettings.h"

using namespace Robomongo;
using ReadPreference = ReplicaSetSettings::ReadPreference;

TEST(ReplicaSetSettingsTests, defaults)
{
    ReplicaSetSettings settings;

    EXPECT_EQ("", settings.setNameUserEntered());
    EXPECT_EQ("", settings.cachedSetName());
    EXPECT_TRUE(settings.members().empty());
    EXPECT_EQ(ReadPreference::PRIMARY, settings.readPreference());
}

TEST(ReplicaSetSettingsTests, roundTripsThroughVariant)
{
    ReplicaSetSettings original;
    original.setSetNameUserEntered("rs0");
    original.setCachedSetName("rs0-cached");
    original.setMembers(std::vector<std::string>{ "a.example.com:27017", "b.example.com:27018" });
    original.setReadPreference(ReadPreference::PRIMARY_PREFERRED);

    ReplicaSetSettings loaded;
    loaded.fromVariant(original.toVariant().toMap());

    EXPECT_EQ("rs0", loaded.setNameUserEntered());
    EXPECT_EQ("rs0-cached", loaded.cachedSetName());
    EXPECT_EQ(original.members(), loaded.members());
    EXPECT_EQ(ReadPreference::PRIMARY_PREFERRED, loaded.readPreference());
}

TEST(ReplicaSetSettingsTests, keepsMemberOrderBeyondTenMembers)
{
    // Members are saved under "0", "1", ... "10"; text ordering must not reorder them
    std::vector<std::string> members;
    for (int i = 0; i < 12; ++i)
        members.push_back("host" + std::to_string(i) + ":27017");
    ReplicaSetSettings original;
    original.setMembers(members);

    ReplicaSetSettings loaded;
    loaded.fromVariant(original.toVariant().toMap());

    EXPECT_EQ(members, loaded.members());
}

TEST(ReplicaSetSettingsTests, stopsReadingMembersAtFirstGap)
{
    QVariantMap map;
    map.insert("0", "a:27017");
    map.insert("1", "b:27017");
    map.insert("3", "d:27017");

    ReplicaSetSettings settings;
    settings.fromVariant(map);

    EXPECT_EQ(std::vector<std::string>({ "a:27017", "b:27017" }), settings.members());
}

TEST(ReplicaSetSettingsTests, setMembersReplacesExistingMembers)
{
    ReplicaSetSettings settings;
    settings.setMembers(std::vector<std::string>{ "a:27017", "b:27017" });

    settings.setMembers(std::vector<std::string>{ "c:27017" });

    EXPECT_EQ(std::vector<std::string>({ "c:27017" }), settings.members());
}

TEST(ReplicaSetSettingsTests, setMembersWithHealthKeepsHostsOnly)
{
    ReplicaSetSettings settings;

    settings.setMembers(std::vector<std::pair<std::string, bool>>{ { "a:27017", true }, { "b:27017", false } });

    EXPECT_EQ(std::vector<std::string>({ "a:27017", "b:27017" }), settings.members());
}

TEST(ReplicaSetSettingsTests, deleteAllMembers)
{
    ReplicaSetSettings settings;
    settings.setMembers(std::vector<std::string>{ "a:27017" });

    settings.deleteAllMembers();

    EXPECT_TRUE(settings.members().empty());
}

TEST(ReplicaSetSettingsTests, convertsMembersToHostAndPort)
{
    ReplicaSetSettings settings;
    settings.setMembers(std::vector<std::string>{ "a.example.com:27017", "b.example.com:27018" });

    std::vector<mongo::HostAndPort> const hosts = settings.membersToHostAndPort();

    ASSERT_EQ(2u, hosts.size());
    EXPECT_EQ("a.example.com", hosts[0].host());
    EXPECT_EQ(27017, hosts[0].port());
    EXPECT_EQ("b.example.com", hosts[1].host());
    EXPECT_EQ(27018, hosts[1].port());
    EXPECT_EQ(2u, settings.membersToHostAndPortAsSet().size());
}

TEST(ReplicaSetSettingsTests, cloneIsIndependentCopy)
{
    ReplicaSetSettings original;
    original.setCachedSetName("rs0");
    original.setMembers(std::vector<std::string>{ "a:27017" });
    std::unique_ptr<ReplicaSetSettings> const copy(original.clone());

    original.setCachedSetName("changed");
    original.deleteAllMembers();

    EXPECT_EQ("rs0", copy->cachedSetName());
    EXPECT_EQ(std::vector<std::string>({ "a:27017" }), copy->members());
}
