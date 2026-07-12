#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>

using Utility::ParseUrl;

TEST(ParseUrl, HttpsWithPathAndDefaultPort)
{
    auto p = ParseUrl("https://uptime.betterstack.com/api/v1/heartbeat/abc123");
    ASSERT_TRUE(p.has_value());
    EXPECT_TRUE(p->https);
    EXPECT_EQ(p->host, "uptime.betterstack.com");
    EXPECT_EQ(p->port, "443");
    EXPECT_EQ(p->target, "/api/v1/heartbeat/abc123");
}

TEST(ParseUrl, FailSuffixIsCarriedIntoTarget)
{
    // This is exactly what the crash path builds: url + "/fail".
    auto p = ParseUrl("https://uptime.betterstack.com/api/v1/heartbeat/abc123/fail");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->host, "uptime.betterstack.com");
    EXPECT_EQ(p->target, "/api/v1/heartbeat/abc123/fail");
}

TEST(ParseUrl, ExplicitPortOverridesDefault)
{
    auto p = ParseUrl("https://example.com:8443/beat");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->host, "example.com");
    EXPECT_EQ(p->port, "8443");
    EXPECT_EQ(p->target, "/beat");
}

TEST(ParseUrl, HttpDefaultsToPort80)
{
    auto p = ParseUrl("http://example.com/x");
    ASSERT_TRUE(p.has_value());
    EXPECT_FALSE(p->https);
    EXPECT_EQ(p->port, "80");
}

TEST(ParseUrl, NoPathYieldsRootTarget)
{
    auto p = ParseUrl("https://example.com");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->host, "example.com");
    EXPECT_EQ(p->target, "/");
}

TEST(ParseUrl, RejectsMissingScheme)
{
    EXPECT_FALSE(ParseUrl("uptime.betterstack.com/x").has_value());
    EXPECT_FALSE(ParseUrl("").has_value());
}

TEST(ParseUrl, RejectsUnknownScheme)
{
    EXPECT_FALSE(ParseUrl("ftp://example.com/x").has_value());
}

TEST(ParseUrl, RejectsEmptyHost)
{
    EXPECT_FALSE(ParseUrl("https:///just/a/path").has_value());
}
