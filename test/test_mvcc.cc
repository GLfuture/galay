
#include <gtest/gtest.h>
#include "galay/galay.hpp"

TEST(MvccTest, PutAndGetCurrentValue) {
    galay::mvcc::Mvcc<int> m;
    int a = 1;
    m.PutValue(&a);
    auto v = m.GetCurrentValue();
    EXPECT_EQ(*v->value, 1);
}

TEST(MvccTest, RemoveValueAndIsValid) {
    galay::mvcc::Mvcc<int> m;
    int a = 1;
    m.PutValue(&a);
    auto v = m.GetCurrentValue();
    m.RemoveValue(v->version);
    EXPECT_FALSE(m.IsValid(v->version));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}