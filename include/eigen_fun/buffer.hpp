#pragma once
// A hand-written owning buffer implementing the Rule of Five. (In real code you
// would use std::vector or std::unique_ptr<T[]> and get all of this for free --
// the point here is to see what those types do for you.)

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace ef::move {

template <class T>
class Buffer {
public:
    Buffer() noexcept = default;

    explicit Buffer(std::size_t n) : data_(n ? new T[n]{} : nullptr), size_(n) {}

    Buffer(std::initializer_list<T> init) : Buffer(init.size()) {
        std::copy(init.begin(), init.end(), data_);
    }

    // 1. Destructor
    ~Buffer() { delete[] data_; }

    // 2. Copy constructor: deep copy.
    Buffer(const Buffer& other) : Buffer(other.size_) {
        std::copy(other.data_, other.data_ + other.size_, data_);
    }

    // 3. Move constructor: steal the pointer, leave `other` empty but valid.
    Buffer(Buffer&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)), size_(std::exchange(other.size_, 0)) {}

    // 4 & 5. Unified assignment via copy-and-swap. Taking the parameter by value
    // means the caller's argument is copy- or move-constructed into `other`,
    // giving strong exception safety and correct self-assignment for free.
    Buffer& operator=(Buffer other) noexcept {
        swap(*this, other);
        return *this;
    }

    friend void swap(Buffer& a, Buffer& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
    }

    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    const T* data() const noexcept { return data_; }
    T* data() noexcept { return data_; }

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }
    T& at(std::size_t i) {
        if (i >= size_) throw std::out_of_range("Buffer::at");
        return data_[i];
    }

private:
    T* data_ = nullptr;
    std::size_t size_ = 0;
};

}  // namespace ef::move
