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

} // namespace
