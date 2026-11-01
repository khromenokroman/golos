#include <gtest/gtest.h>

#include "text_util.hpp"

namespace
{

TEST(Trim, Empty)
{
    EXPECT_EQ(trim(""), "");
}

TEST(Trim, OnlySpaces)
{
    EXPECT_EQ(trim(" \t\r\n "), "");
}

TEST(Trim, BothEnds)
{
    EXPECT_EQ(trim("  привет мир\t\n"), "привет мир");
}

TEST(Trim, InnerUntouched)
{
    EXPECT_EQ(trim("a  b"), "a  b");
}

TEST(TypingDiff, AppendOnly)
{
    const auto [bs, suffix] = typing_diff("раз два", "раз два три");
    EXPECT_EQ(bs, 0u);
    EXPECT_EQ(suffix, " три");
}

TEST(TypingDiff, NoChanges)
{
    const auto [bs, suffix] = typing_diff("раз два", "раз два");
    EXPECT_EQ(bs, 0u);
    EXPECT_EQ(suffix, "");
}

TEST(TypingDiff, RevisionInMiddle)
{
    // Whisper пересмотрел слово: "два" -> "двадцать"
    const auto [bs, suffix] = typing_diff("раз два три", "раз двадцать три");
    EXPECT_EQ(bs, 4u);
    EXPECT_EQ(suffix, "дцать три");
}

TEST(TypingDiff, FullRewrite)
{
    const auto [bs, suffix] = typing_diff("старый текст", "новый");
    EXPECT_EQ(bs, 12u);
    EXPECT_EQ(suffix, "новый");
}

TEST(TypingDiff, EmptyTyped)
{
    const auto [bs, suffix] = typing_diff("", "привет");
    EXPECT_EQ(bs, 0u);
    EXPECT_EQ(suffix, "привет");
}

} // namespace
