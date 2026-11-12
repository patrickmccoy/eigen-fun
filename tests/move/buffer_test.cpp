#include "eigen_fun/buffer.hpp"

#include <gtest/gtest.h>
#include <type_traits>
#include <vector>

using ef::move::Buffer;

static_assert(std::is_nothrow_move_constructible_v<Buffer<int>>);
static_assert(std::is_nothrow_move_assignable_v<Buffer<int>>);

TEST(Buffer, CopyIsDeep) {
    Buffer<int> a{1, 2, 3};
    Buffer<int> b = a;
    EXPECT_NE(a.data(), b.data());
    b[0] = 99;
    EXPECT_EQ(a[0], 1);
}

TEST(Buffer, MoveStealsStorageAndEmptiesSource) {
    Buffer<int> a{1, 2, 3};
    const int* storage = a.data();
    Buffer<int> b = std::move(a);
    EXPECT_EQ(b.data(), storage);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(a.data(), nullptr);
}

TEST(Buffer, MoveAssignmentReleasesOldStorage) {
    Buffer<int> a{1, 2, 3};
    Buffer<int> b{4, 5};
    const int* storage = a.data();
    b = std::move(a);
    EXPECT_EQ(b.data(), storage);
    EXPECT_EQ(b[2], 3);
    // b's old storage was moved into the by-value parameter and freed at the end
    // of operator= (run under -DEF_SANITIZE=address to prove there is no leak).
}

TEST(Buffer, SelfAssignmentIsSafe) {
    Buffer<int> a{1, 2, 3};
    auto& alias = a;  // avoid -Wself-assign-overloaded
    a = alias;
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a[1], 2);
    a = std::move(alias);
    EXPECT_EQ(a.size(), 3u);
}

TEST(Buffer, MovedFromObjectIsReusable) {
    Buffer<int> a{1, 2, 3};
    Buffer<int> b = std::move(a);
    a = Buffer<int>{7, 8};
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a[0], 7);
}

TEST(Buffer, WorksInsideContainers) {
    std::vector<Buffer<double>> v;
    for (int i = 0; i < 100; ++i) v.emplace_back(static_cast<std::size_t>(1000));
    std::vector<const double*> before;
    for (auto& b : v) before.push_back(b.data());
    v.reserve(v.capacity() * 2);  // reallocation moves (noexcept) -> pointers survive
    for (std::size_t i = 0; i < v.size(); ++i) EXPECT_EQ(v[i].data(), before[i]);
}

TEST(Buffer, AtIsBoundsChecked) {
    Buffer<int> a(2);
    EXPECT_EQ(a.at(1), 0);  // value-initialised
    EXPECT_THROW(a.at(2), std::out_of_range);
}
