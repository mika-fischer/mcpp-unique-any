// Copyright Mika Fischer 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace mcpp {

class bad_inplace_unique_any_cast : public std::bad_cast {
  public:
    [[nodiscard]] auto what() const noexcept -> const char * override { return "bad_inplace_unique_any_cast"; }
};

namespace detail {
struct inplace_unique_any_vtable {
    void (*destroy)(void *buf) noexcept;
    void (*move)(void *dst, void *src) noexcept;
    const std::type_info *type_info;
};

template <typename T>
    requires std::is_nothrow_move_constructible_v<T>
auto vtable_for() -> const inplace_unique_any_vtable & {
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    static constexpr auto vtable = inplace_unique_any_vtable{
        .destroy = [](void *buf) noexcept -> void { reinterpret_cast<T *>(buf)->~T(); },
        .move = [](void *dst, void *src) noexcept -> void { new (dst) T(std::move(*reinterpret_cast<T *>(src))); },
        .type_info = &typeid(T),
    };
    return vtable;
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}
} // namespace detail

template <size_t SIZE, size_t ALIGN>
class inplace_unique_any { // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    alignas(ALIGN) std::array<std::byte, SIZE> buffer_;
    const detail::inplace_unique_any_vtable *vtable_ = nullptr;

  public:
    // https://en.cppreference.com/w/cpp/utility/any/any.html
    // (1)
    constexpr inplace_unique_any() = default;
    // (2)
    inplace_unique_any(const inplace_unique_any &) = delete;
    // (3)
    inplace_unique_any(
        inplace_unique_any &&other) noexcept { // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
        if (other.vtable_ != nullptr) {
            other.vtable_->move(buffer_.data(), other.buffer_.data());
            vtable_ = std::exchange(other.vtable_, nullptr);
            vtable_->destroy(other.buffer_.data());
        }
    }
    // (4)
    template <typename U>
        requires(!std::same_as<std::decay_t<U>, inplace_unique_any> &&
                 std::is_nothrow_move_constructible_v<std::decay_t<U>>)
    // NOLINTNEXTLINE(hicpp-explicit-conversions,cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    inplace_unique_any(U &&value) {
        using T = std::decay_t<U>;
        static_assert(sizeof(T) <= SIZE, "Type is too large for inplace_unique_any buffer.");
        static_assert(alignof(T) <= ALIGN, "Type alignment is too large for inplace_unique_any buffer.");
        new (buffer_.data()) T(std::forward<U>(value));
        vtable_ = &detail::vtable_for<T>(); // NOLINT(cppcoreguidelines-prefer-member-initializer)
    }
    // (5)
    template <class ValueType, class... Args>
        requires(std::is_constructible_v<std::decay_t<ValueType>, Args...> &&
                 std::is_nothrow_move_constructible_v<std::decay_t<ValueType>>)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    explicit inplace_unique_any(std::in_place_type_t<ValueType> /*unused*/, Args &&...args) {
        using T = std::decay_t<ValueType>;
        static_assert(sizeof(T) <= SIZE, "Type is too large for inplace_unique_any buffer.");
        static_assert(alignof(T) <= ALIGN, "Type alignment is too large for inplace_unique_any buffer.");
        new (buffer_.data()) T(std::forward<Args>(args)...);
        vtable_ = &detail::vtable_for<T>(); // NOLINT(cppcoreguidelines-prefer-member-initializer)
    }
    // (6)
    template <class ValueType, class U, class... Args>
        requires(std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...> &&
                 std::is_nothrow_move_constructible_v<std::decay_t<ValueType>>)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    explicit inplace_unique_any(std::in_place_type_t<ValueType> /*unused*/, std::initializer_list<U> il,
                                Args &&...args) {
        using T = std::decay_t<ValueType>;
        static_assert(sizeof(T) <= SIZE, "Type is too large for inplace_unique_any buffer.");
        static_assert(alignof(T) <= ALIGN, "Type alignment is too large for inplace_unique_any buffer.");
        new (buffer_.data()) T(il, std::forward<Args>(args)...);
        vtable_ = &detail::vtable_for<T>(); // NOLINT(cppcoreguidelines-prefer-member-initializer)
    }

    // https://en.cppreference.com/w/cpp/utility/any/operator=.html
    // (1)
    auto operator=(const inplace_unique_any &) -> inplace_unique_any & = delete;
    // (2)
    auto operator=(inplace_unique_any &&other) noexcept -> inplace_unique_any & {
        if (this != &other) {
            reset();
            if (other.vtable_ != nullptr) {
                other.vtable_->move(buffer_.data(), other.buffer_.data());
                vtable_ = std::exchange(other.vtable_, nullptr);
                vtable_->destroy(other.buffer_.data());
            }
        }
        return *this;
    }
    // (3)
    template <typename U>
        requires(!std::same_as<std::decay_t<U>, inplace_unique_any> &&
                 std::is_nothrow_move_constructible_v<std::decay_t<U>>)
    auto operator=(U &&value) -> inplace_unique_any & {
        using T = std::decay_t<U>;
        static_assert(sizeof(T) <= SIZE, "Type is too large for inplace_unique_any buffer.");
        static_assert(alignof(T) <= ALIGN, "Type alignment is too large for inplace_unique_any buffer.");
        reset();
        new (buffer_.data()) T(std::forward<U>(value));
        vtable_ = &detail::vtable_for<T>(); // NOLINT(cppcoreguidelines-prefer-member-initializer)
        return *this;
    }

    // https://en.cppreference.com/w/cpp/utility/any/~any.html
    ~inplace_unique_any() { reset(); }

    // https://en.cppreference.com/w/cpp/utility/any/emplace.html
    // (1)
    template <class ValueType, class... Args>
        requires(std::is_constructible_v<std::decay_t<ValueType>, Args...> and
                 std::is_nothrow_move_constructible_v<std::decay_t<ValueType>>)
    auto emplace(Args &&...args) -> std::decay_t<ValueType> & {
        using T = std::decay_t<ValueType>;
        static_assert(sizeof(T) <= SIZE, "Type is too large for inplace_unique_any buffer.");
        static_assert(alignof(T) <= ALIGN, "Type alignment is too large for inplace_unique_any buffer.");
        reset();
        auto *val = new (buffer_.data()) T(std::forward<Args>(args)...); // NOLINT(cppcoreguidelines-owning-memory)
        vtable_ = &detail::vtable_for<T>();
        return *val;
    }
    // (2)
    template <class ValueType, class U, class... Args>
        requires(std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...> and
                 std::is_nothrow_move_constructible_v<std::decay_t<ValueType>>)
    auto emplace(std::initializer_list<U> il, Args &&...args) -> std::decay_t<ValueType> & {
        using T = std::decay_t<ValueType>;
        static_assert(sizeof(T) <= SIZE, "Type is too large for inplace_unique_any buffer.");
        static_assert(alignof(T) <= ALIGN, "Type alignment is too large for inplace_unique_any buffer.");
        reset();
        auto *val = new (buffer_.data()) T(il, std::forward<Args>(args)...); // NOLINT(cppcoreguidelines-owning-memory)
        vtable_ = &detail::vtable_for<T>();
        return *val;
    }

    // https://en.cppreference.com/w/cpp/utility/any/reset.html
    void reset() noexcept {
        if (vtable_ != nullptr) {
            vtable_->destroy(buffer_.data());
            vtable_ = nullptr;
        }
    }

    // https://en.cppreference.com/w/cpp/utility/any/swap.html
    void swap(inplace_unique_any &other) noexcept {
        if (this == &other || (!this->has_value() && !other.has_value())) {
            return;
        }
        if (!this->has_value()) {
            *this = std::move(other);
            return;
        }
        if (!other.has_value()) {
            other = std::move(*this);
            return;
        }
        alignas(ALIGN) std::array<std::byte, SIZE> temp_buffer;
        // this: A, other: B, temp: <uninitialized>
        this->vtable_->move(temp_buffer.data(), this->buffer_.data());
        // this: A(moved-from), other: B, temp: A
        this->vtable_->destroy(this->buffer_.data());
        // this: <uninitialized>, other: B, temp: A
        other.vtable_->move(this->buffer_.data(), other.buffer_.data());
        // this: B, other: B(moved-from), temp: A
        other.vtable_->destroy(other.buffer_.data());
        // this: B, other: <uninitialized>, temp: A
        this->vtable_->move(other.buffer_.data(), temp_buffer.data());
        // this: B, other: A, temp: A(moved-from)
        this->vtable_->destroy(temp_buffer.data());
        // this: B, other: A, temp: <uninitialized>
        std::swap(this->vtable_, other.vtable_);
    }

    // https://en.cppreference.com/w/cpp/utility/any/has_value.html
    [[nodiscard]] auto has_value() const noexcept -> bool { return vtable_ != nullptr; }

    // https://en.cppreference.com/w/cpp/utility/any/type.html
    [[nodiscard]] auto type() const noexcept -> const std::type_info & {
        return vtable_ != nullptr ? *vtable_->type_info : typeid(void);
    }

    // https://en.cppreference.com/w/cpp/utility/any/swap2.html
    friend void swap(inplace_unique_any &lhs, inplace_unique_any &rhs) noexcept { return lhs.swap(rhs); }

    template <class T, size_t S, size_t A>
    friend auto any_cast(const inplace_unique_any<S, A> *operand) noexcept -> const T *;

    template <class T, size_t S, size_t A>
    friend auto any_cast(inplace_unique_any<S, A> *operand) noexcept -> T *;
};

// https://en.cppreference.com/w/cpp/utility/any/any_cast.html
// (1)
template <class T, size_t S, size_t A>
auto any_cast(const inplace_unique_any<S, A> &operand) -> T {
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, const U &>);
    if (auto *ptr = any_cast<U>(&operand)) {
        return static_cast<T>(*ptr);
    }
    throw bad_inplace_unique_any_cast();
}
// (2)
template <class T, size_t S, size_t A>
auto any_cast(inplace_unique_any<S, A> &operand) -> T {
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, U &>);
    if (auto *ptr = any_cast<U>(&operand)) {
        return static_cast<T>(*ptr);
    }
    throw bad_inplace_unique_any_cast();
}
// (3)
template <class T, size_t S, size_t A>
auto any_cast(inplace_unique_any<S, A> &&operand) -> T { // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, U>);
    if (auto *ptr = any_cast<U>(&operand)) {
        return static_cast<T>(std::move(*ptr));
    }
    throw bad_inplace_unique_any_cast();
}
// (4)
template <class T, size_t S, size_t A>
auto any_cast(const inplace_unique_any<S, A> *operand) noexcept -> const T * {
    static_assert(!std::is_void_v<T>);
    return operand && operand->type() == typeid(T)
               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
               ? reinterpret_cast<const T *>(operand->buffer_.data())
               : nullptr;
}
// (5)
template <class T, size_t S, size_t A>
auto any_cast(inplace_unique_any<S, A> *operand) noexcept -> T * {
    static_assert(!std::is_void_v<T>);
    return operand && operand->type() == typeid(T)
               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
               ? reinterpret_cast<T *>(operand->buffer_.data())
               : nullptr;
}

} // namespace mcpp