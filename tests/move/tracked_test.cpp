#include "eigen_fun/tracked.hpp"

#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

using namespace ef::move;

class TrackedTest : public ::testing::Test {
protected:
    Counters c;
};

TEST_F(TrackedTest, StdMoveIsJustACast) {
    Tracked a(c, 42);
    Tracked&& ref = std::move(a);  // no move happens here...
    EXPECT_EQ(c.moves(), 0);
    Tracked b(std::move(ref));  // ...only when a move constructor is invoked
    EXPECT_EQ(c.moveConstructs, 1);
    EXPECT_EQ(b.value(), 42);
    EXPECT_EQ(a.value(), -1);  // our type documents its moved-from state
}

TEST_F(TrackedTest, ConstObjectsAreSilentlyCopiedNotMoved) {
    const Tracked a(c, 1);
    Tracked b(std::move(a));  // const Tracked&& binds to the copy ctor
    EXPECT_EQ(c.copyConstructs, 1);
    EXPECT_EQ(c.moves(), 0);
}

TEST_F(TrackedTest, GuaranteedCopyElisionForPrvalues) {
    auto make = [this] { return Tracked(c, 7); };  // C++17: guaranteed elision
    Tracked t = make();
    EXPECT_EQ(c.constructs, 1);
    EXPECT_EQ(c.copies(), 0);
    EXPECT_EQ(c.moves(), 0);
    EXPECT_EQ(t.value(), 7);
}

TEST_F(TrackedTest, NamedReturnValueIsElidedOrMovedNeverCopied) {
    auto make = [this] {
        Tracked local(c, 7);
        return local;  // NRVO is optional; if skipped, `local` is implicitly moved
    };
    Tracked t = make();
    EXPECT_EQ(c.copies(), 0);
    EXPECT_LE(c.moves(), 1);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpessimizing-move"
TEST_F(TrackedTest, ReturningStdMovePessimizesByDisablingNRVO) {
    auto make = [this] {
        Tracked local(c, 7);
        return std::move(local);  // don't do this: forces a move
    };
    Tracked t = make();
    EXPECT_EQ(c.moveConstructs, 1);
}
#pragma clang diagnostic pop

TEST_F(TrackedTest, SinkByValueCostsOneMoveForRvalues) {
    Tracked lvalue(c, 1);
    c.reset();

    Holder h1(lvalue);  // copy into parameter, move into member
    EXPECT_EQ(c.copyConstructs, 1);
    EXPECT_EQ(c.moveConstructs, 1);
    c.reset();

    Holder h2(Tracked(c, 2));  // prvalue materialises in the parameter, then 1 move
    EXPECT_EQ(c.copies(), 0);
    EXPECT_EQ(c.moveConstructs, 1);
    c.reset();

    h2.set(std::move(lvalue));  // move into parameter, move-assign into member
    EXPECT_EQ(c.copies(), 0);
    EXPECT_EQ(c.moveConstructs, 1);
    EXPECT_EQ(c.moveAssigns, 1);
    EXPECT_EQ(h2.get().value(), 1);
}

TEST_F(TrackedTest, PerfectForwardingPreservesValueCategory) {
    Tracked src(c, 5);
    c.reset();

    auto copy = makeForwarded<Tracked>(src);  // lvalue stays an lvalue
    EXPECT_EQ(c.copyConstructs, 1);
    EXPECT_EQ(c.moves(), 0);
    c.reset();

    auto moved = makeForwarded<Tracked>(std::move(src));  // rvalue stays an rvalue
    EXPECT_EQ(c.copies(), 0);
    EXPECT_EQ(c.moveConstructs, 1);
}

TEST_F(TrackedTest, EmplaceBackConstructsInPlace) {
    std::vector<Tracked> v;
    v.reserve(4);
    c.reset();

    v.push_back(Tracked(c, 1));  // construct temporary + move
    EXPECT_EQ(c.constructs, 1);
    EXPECT_EQ(c.moveConstructs, 1);
    c.reset();

    v.emplace_back(c, 2);  // constructed directly in the vector's storage
    EXPECT_EQ(c.constructs, 1);
    EXPECT_EQ(c.moves(), 0);
}

TEST_F(TrackedTest, VectorReallocationMovesOnlyIfMoveIsNoexcept) {
    static_assert(std::is_nothrow_move_constructible_v<Tracked>);
    static_assert(!std::is_nothrow_move_constructible_v<ThrowingMoveTracked>);

    {
        std::vector<Tracked> v;
        v.reserve(4);
        for (int i = 0; i < 4; ++i) v.emplace_back(c, i);
        c.reset();
        v.emplace_back(c, 4);  // forces reallocation of 4 existing elements
        EXPECT_EQ(c.moveConstructs, 4);
        EXPECT_EQ(c.copyConstructs, 0);
    }

    c.reset();
    {
        // Without noexcept the vector must copy (std::move_if_noexcept) to keep
        // its strong exception-safety guarantee.
        std::vector<ThrowingMoveTracked> v;
        v.reserve(4);
        for (int i = 0; i < 4; ++i) v.emplace_back(c, i);
        c.reset();
        v.emplace_back(c, 4);
        EXPECT_EQ(c.copyConstructs, 4);
        EXPECT_EQ(c.moveConstructs, 0);
    }
}

TEST(MoveStdTypes, MovedFromStringIsValidButUnspecified) {
    std::string a(100, 'x');  // long enough to defeat the small-string optimisation
    const char* buffer = a.data();
    std::string b = std::move(a);
    EXPECT_EQ(b.data(), buffer);  // heap buffer was stolen, not copied
    a = "reusable";               // a moved-from object can always be assigned to
    EXPECT_EQ(a, "reusable");
}

TEST(MoveStdTypes, MovedFromOptionalStillHoldsAValue) {
    std::optional<std::string> a = std::string(100, 'x');
    std::optional<std::string> b = std::move(a);
    EXPECT_TRUE(a.has_value());  // surprising! the *contained* string was moved
    EXPECT_TRUE(b.has_value());
}
