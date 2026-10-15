#include <gtest/gtest.h>

#include <mongocxx/uri.hpp>

#include "robomongo/core/mongodb/modern/ModernConnectionUri.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"

using namespace Robomongo;

namespace
{
    std::string uriFor(const std::string &host, int port, const std::string &user = "",
                       const std::string &password = "", const std::string &db = "admin",
                       bool enabled = true)
    {
        ConnectionSettings settings(false);
        settings.setServerHost(host);
        settings.setServerPort(port);
        if (!user.empty() || !password.empty()) {
            auto credential = new CredentialSettings();
            credential->setUserName(user);
            credential->setUserPassword(password);
            credential->setDatabaseName(db);
            credential->setEnabled(enabled);
            settings.addCredential(credential);
        }
        return modernDirectUri(settings);
    }
}

TEST(ModernConnectionUriTests, hostAndPortWithoutCredentials)
{
    EXPECT_EQ("mongodb://localhost:27017", uriFor("localhost", 27017));
    EXPECT_EQ("mongodb://db.example.com:27018", uriFor("db.example.com", 27018));
}

TEST(ModernConnectionUriTests, includesCredentialsAndAuthSource)
{
    EXPECT_EQ("mongodb://ada:secret@localhost:27017/?authSource=admin",
              uriFor("localhost", 27017, "ada", "secret"));
}

TEST(ModernConnectionUriTests, usesCredentialDatabaseAsAuthSource)
{
    EXPECT_EQ("mongodb://ada:secret@localhost:27017/?authSource=shop",
              uriFor("localhost", 27017, "ada", "secret", "shop"));
}

TEST(ModernConnectionUriTests, disabledCredentialIsLeftOut)
{
    EXPECT_EQ("mongodb://localhost:27017", uriFor("localhost", 27017, "ada", "secret", "admin", false));
}

TEST(ModernConnectionUriTests, percentEncodesReservedCharactersInCredentials)
{
    // ':' '@' '/' '?' '#' '%' would otherwise break the URI or change its meaning
    EXPECT_EQ("mongodb://us%3Aer%40x:p%40ss%3Aw%2Fo%3Fr%23d%25@localhost:27017/?authSource=admin",
              uriFor("localhost", 27017, "us:er@x", "p@ss:w/o?r#d%"));
}

TEST(ModernConnectionUriTests, keepsUnreservedCharacters)
{
    EXPECT_EQ("mongodb://a-b_c.d~e:Az09-_.~@localhost:27017/?authSource=admin",
              uriFor("localhost", 27017, "a-b_c.d~e", "Az09-_.~"));
}

TEST(ModernConnectionUriTests, percentEncodesSpacesAndUnicode)
{
    EXPECT_EQ("mongodb://ad%20a:p%C3%A4ss@localhost:27017/?authSource=admin",
              uriFor("localhost", 27017, "ad a", "päss"));
}

TEST(ModernConnectionUriTests, percentEncodesAuthSource)
{
    EXPECT_EQ("mongodb://ada:secret@localhost:27017/?authSource=my%26db",
              uriFor("localhost", 27017, "ada", "secret", "my&db"));
}

TEST(ModernConnectionUriTests, encodedCredentialsSurviveDriverParsing)
{
    std::string const uri = uriFor("localhost", 27017, "us:er@x", "p@ss:w/o?r#d% ü", "shop");

    mongocxx::uri const parsed(uri);

    EXPECT_EQ("us:er@x", parsed.username());
    EXPECT_EQ("p@ss:w/o?r#d% ü", parsed.password());
    EXPECT_EQ("shop", parsed.auth_source());
    ASSERT_EQ(1u, parsed.hosts().size());
    EXPECT_EQ("localhost", parsed.hosts()[0].name);
    EXPECT_EQ(27017, parsed.hosts()[0].port);
}

TEST(ModernConnectionUriTests, bracketedIpv6HostIsAccepted)
{
    std::string const uri = uriFor("[::1]", 27017);

    EXPECT_EQ("mongodb://[::1]:27017", uri);
    mongocxx::uri const parsed(uri);
    EXPECT_EQ(27017, parsed.hosts()[0].port);
}
