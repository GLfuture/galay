#include "galay/galay.hpp"
#include <gtest/gtest.h>

TEST(Trie, test)
{
    galay::utils::TrieTree tree;
    tree.Add("hello");
    tree.Add("world");
    tree.Add("hi");
    ASSERT_TRUE(tree.Contains("hello"));
    ASSERT_TRUE(tree.Contains("world"));
    ASSERT_TRUE(tree.Contains("hi"));
    ASSERT_FALSE(tree.Contains("hell"));
    ASSERT_EQ(tree.Query("hello"), 1);
    auto res = tree.GetWordByPrefix("h");
    ASSERT_EQ(res, std::vector<std::string>({"hello", "hi"}));
    tree.Remove("hello");
    ASSERT_FALSE(tree.Contains("hello"));
}