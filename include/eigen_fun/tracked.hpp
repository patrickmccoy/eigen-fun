#pragma once
// A value type that records every special-member-function call made on it, so
// tests can assert exactly how many copies and moves an operation performed.

#include <ostream>
#include <utility>

namespace ef::move {

struct Counters {
    int constructs = 0;
    int copyConstructs = 0;
    int moveConstructs = 0;
    int copyAssigns = 0;
    int moveAssigns = 0;
    int destructs = 0;

    int copies() const { return copyConstructs + copyAssigns; }
    int moves() const { return moveConstructs + moveAssigns; }
    void reset() { *this = Counters{}; }
};

inline std::ostream& operator<<(std::ostream& os, const Counters& c) {
    return os << "{ctor=" << c.constructs << " copy=" << c.copyConstructs << "+" << c.copyAssigns
              << " move=" << c.moveConstructs << "+" << c.moveAssigns << " dtor=" << c.destructs << "}";
}

// NoexceptMove lets tests compare containers' behaviour with and without a
// noexcept move constructor (std::vector only moves on reallocation if noexcept).
template <bool NoexceptMove = true>
class BasicTracked {
public:
    BasicTracked(Counters& c, int value) : c_(&c), value_(value) { ++c_->constructs; }
    BasicTracked(const BasicTracked& o) : c_(o.c_), value_(o.value_) { ++c_->copyConstructs; }
    BasicTracked(BasicTracked&& o) noexcept(NoexceptMove) : c_(o.c_), value_(std::exchange(o.value_, -1)) {
        ++c_->moveConstructs;
    }
    BasicTracked& operator=(const BasicTracked& o) {
        c_ = o.c_;
        value_ = o.value_;
        ++c_->copyAssigns;
        return *this;
    }
    BasicTracked& operator=(BasicTracked&& o) noexcept(NoexceptMove) {
        c_ = o.c_;
        value_ = std::exchange(o.value_, -1);
        ++c_->moveAssigns;
        return *this;
    }
    ~BasicTracked() { ++c_->destructs; }

    int value() const { return value_; }  // -1 means "moved-from"

private:
    Counters* c_;
    int value_;
};

using Tracked = BasicTracked<true>;
using ThrowingMoveTracked = BasicTracked<false>;

// The "pass by value, then move" sink idiom: one overload serves both lvalues
// (1 copy + 1 move) and rvalues (2 moves).
class Holder {
public:
    explicit Holder(Tracked t) : held_(std::move(t)) {}
    void set(Tracked t) { held_ = std::move(t); }
    const Tracked& get() const { return held_; }

private:
    Tracked held_;
};

// Perfect forwarding: preserves the value category of each argument.
template <class T, class... Args>
T makeForwarded(Args&&... args) {
    return T(std::forward<Args>(args)...);
}

}  // namespace ef::move
