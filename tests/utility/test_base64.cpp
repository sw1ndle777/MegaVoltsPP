#include <gtest/gtest.h>
#include <common/BaseLib/Utility.h>
#include <string>
#include <vector>

TEST(Base64, RoundtripSimple)
{
    std::string original = "Hello, World!";
    auto encoded = Utility::Base64::to_base64(original);
    auto decoded = Utility::Base64::from_base64(encoded);
    EXPECT_EQ(decoded, original);
}

TEST(Base64, RoundtripEmpty)
{
    std::string original;
    auto encoded = Utility::Base64::to_base64(original);
    auto decoded = Utility::Base64::from_base64(encoded);
    EXPECT_EQ(decoded, original);
}

TEST(Base64, RoundtripSingleChar)
{
    std::string original = "A";
    auto encoded = Utility::Base64::to_base64(original);
    auto decoded = Utility::Base64::from_base64(encoded);
    EXPECT_EQ(decoded, original);
}

TEST(Base64, RoundtripTwoChars)
{
    std::string original = "AB";
    auto encoded = Utility::Base64::to_base64(original);
    auto decoded = Utility::Base64::from_base64(encoded);
    EXPECT_EQ(decoded, original);
}

TEST(Base64, RoundtripThreeChars)
{
    std::string original = "ABC";
    auto encoded = Utility::Base64::to_base64(original);
    auto decoded = Utility::Base64::from_base64(encoded);
    EXPECT_EQ(decoded, original);
}

TEST(Base64, RFC4648Vector1)
{
    EXPECT_EQ(Utility::Base64::to_base64(""), "");
}

TEST(Base64, RFC4648Vector2)
{
    EXPECT_EQ(Utility::Base64::to_base64("f"), "Zg==");
}

TEST(Base64, RFC4648Vector3)
{
    EXPECT_EQ(Utility::Base64::to_base64("fo"), "Zm8=");
}

TEST(Base64, RFC4648Vector4)
{
    EXPECT_EQ(Utility::Base64::to_base64("foo"), "Zm9v");
}

TEST(Base64, RFC4648Vector5)
{
    EXPECT_EQ(Utility::Base64::to_base64("foob"), "Zm9vYg==");
}

TEST(Base64, RFC4648Vector6)
{
    EXPECT_EQ(Utility::Base64::to_base64("fooba"), "Zm9vYmE=");
}

TEST(Base64, RFC4648Vector7)
{
    EXPECT_EQ(Utility::Base64::to_base64("foobar"), "Zm9vYmFy");
}

TEST(Base64, DecodeRFC4648)
{
    EXPECT_EQ(Utility::Base64::from_base64("Zg=="), "f");
    EXPECT_EQ(Utility::Base64::from_base64("Zm8="), "fo");
    EXPECT_EQ(Utility::Base64::from_base64("Zm9v"), "foo");
    EXPECT_EQ(Utility::Base64::from_base64("Zm9vYg=="), "foob");
    EXPECT_EQ(Utility::Base64::from_base64("Zm9vYmE="), "fooba");
    EXPECT_EQ(Utility::Base64::from_base64("Zm9vYmFy"), "foobar");
}

TEST(Base64, InvalidCharacterThrows)
{
    EXPECT_THROW(Utility::Base64::from_base64("!!!"), std::runtime_error);
}

TEST(Base64, InvalidPaddingThrows)
{
    EXPECT_THROW(Utility::Base64::from_base64("Z==="), std::runtime_error);
}

TEST(Base64, RoundtripBinaryData)
{
    std::string binary;
    for (int i = 0; i < 256; ++i)
        binary.push_back(static_cast<char>(i));

    auto encoded = Utility::Base64::to_base64(binary);
    auto decoded = Utility::Base64::from_base64(encoded);
    EXPECT_EQ(decoded, binary);
}

TEST(Base64, RoundtripLargeData)
{
    std::string large(4096, 'X');
    auto encoded = Utility::Base64::to_base64(large);
    auto decoded = Utility::Base64::from_base64(encoded);
    EXPECT_EQ(decoded, large);
}
