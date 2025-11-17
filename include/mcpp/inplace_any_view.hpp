// Copyright Mika Fischer 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace mcpp {

class bad_inplace_any_view_cast : public std::bad_cast {
  public:
    [[nodiscard]] auto what() const noexcept -> const char * override { return "bad_inplace_any_view_cast"; }
};

namespace detail {
struct inplace_any_view_vtable {
    void (*destroy)(void *buf) noexcept;
    void (*move)(void *dst, void *src) noexcept;
    const std::type_info *type_info;
};

template <typename T>
    requires std::is_nothrow_move_constructible_v<T>
auto vtable_for() -> const inplace_any_view_vtable & {
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    static constexpr auto vtable = inplace_any_view_vtable{
        .destroy = [](void *buf) noexcept -> void { reinterpret_cast<T *>(buf)->~T(); },
        .move = [](void *dst, void *src) noexcept -> void { new (dst) T(std::move(*reinterpret_cast<T *>(src))); },
        .type_info = &typeid(T),
    };
    return vtable;
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}
} // namespace detail

class inplace_any_view { // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    std::byte *buffer_;
    size_t size_;
    const detail::inplace_any_view_vtable *vtable_ = nullptr;

  public:
    // https://en.cppreference.com/w/cpp/utility/any/any.html
    // (1)
    constexpr inplace_any_view(std::byte *buffer, size_t size) : buffer_(buffer), size_(size) {}
    // (2)
    inplace_any_view(const inplace_any_view &) = delete;
    // (3)
    inplace_any_view(inplace_any_view &&other) noexcept = delete;
    // (4)
    // TODO
    // (5)
    // TODO
    // (6)
    // TODO

    // https://en.cppreference.com/w/cpp/utility/any/operator=.html
    // (1)
    auto operator=(const inplace_any_view &) -> inplace_any_view & = delete;
    // (2)
    // TODO
    // (3)
    template <typename U>
        requires(!std::same_as<std::decay_t<U>, inplace_any_view> &&
                 std::is_nothrow_move_constructible_v<std::decay_t<U>>)
    auto operator=(U &&value) -> inplace_any_view & {
        using T = std::decay_t<U>;
        if (sizeof(T) > size_) {
            throw std::runtime_error("Type is too large for inplace_any_view buffer.");
        }
        if ((reinterpret_cast<std::size_t>(buffer_) & (alignof(T) - 1)) != 0) {
            throw std::runtime_error("Type alignment is too large for inplace_any_view buffer.");
        }
        reset();
        new (buffer_) T(std::forward<U>(value));
        vtable_ = &detail::vtable_for<T>(); // NOLINT(cppcoreguidelines-prefer-member-initializer)
        return *this;
    }

    // https://en.cppreference.com/w/cpp/utility/any/~any.html
    ~inplace_any_view() { reset(); }

    // https://en.cppreference.com/w/cpp/utility/any/emplace.html
    // (1)
    template <class ValueType, class... Args>
        requires(std::is_constructible_v<std::decay_t<ValueType>, Args...> and
                 std::is_nothrow_move_constructible_v<std::decay_t<ValueType>>)
    auto emplace(Args &&...args) -> std::decay_t<ValueType> & {
        using T = std::decay_t<ValueType>;
        if (sizeof(T) > size_) {
            throw std::runtime_error("Type is too large for inplace_any_view buffer.");
        }
        if ((reinterpret_cast<std::size_t>(buffer_) & (alignof(T) - 1)) != 0) {
            throw std::runtime_error("Type alignment is too large for inplace_any_view buffer.");
        }
        reset();
        auto *val = new (buffer_) T(std::forward<Args>(args)...); // NOLINT(cppcoreguidelines-owning-memory)
        vtable_ = &detail::vtable_for<T>();
        return *val;
    }
    // (2)
    template <class ValueType, class U, class... Args>
        requires(std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...> and
                 std::is_nothrow_move_constructible_v<std::decay_t<ValueType>>)
    auto emplace(std::initializer_list<U> il, Args &&...args) -> std::decay_t<ValueType> & {
        using T = std::decay_t<ValueType>;
        if (sizeof(T) > size_) {
            throw std::runtime_error("Type is too large for inplace_any_view buffer.");
        }
        if ((reinterpret_cast<std::size_t>(buffer_) & (alignof(T) - 1)) != 0) {
            throw std::runtime_error("Type alignment is too large for inplace_any_view buffer.");
        }
        reset();
        auto *val = new (buffer_) T(il, std::forward<Args>(args)...); // NOLINT(cppcoreguidelines-owning-memory)
        vtable_ = &detail::vtable_for<T>();
        return *val;
    }

    // https://en.cppreference.com/w/cpp/utility/any/reset.html
    void reset() noexcept {
        if (vtable_ != nullptr) {
            vtable_->destroy(buffer_);
            vtable_ = nullptr;
        }
    }

    // https://en.cppreference.com/w/cpp/utility/any/swap.html
    // TODO

    // https://en.cppreference.com/w/cpp/utility/any/has_value.html
    [[nodiscard]] auto has_value() const noexcept -> bool { return vtable_ != nullptr; }

    // https://en.cppreference.com/w/cpp/utility/any/type.html
    [[nodiscard]] auto type() const noexcept -> const std::type_info & {
        return vtable_ != nullptr ? *vtable_->type_info : typeid(void);
    }

    // https://en.cppreference.com/w/cpp/utility/any/swap2.html
    // TODO

    template <class T>
    friend auto any_cast(const inplace_any_view *operand) noexcept -> const T *;

    template <class T>
    friend auto any_cast(inplace_any_view *operand) noexcept -> T *;
};

// https://en.cppreference.com/w/cpp/utility/any/any_cast.html
// (1)
template <class T>
auto any_cast(const inplace_any_view &operand) -> T {
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, const U &>);
    if (auto *ptr = any_cast<U>(&operand)) {
        return static_cast<T>(*ptr);
    }
    throw bad_inplace_any_view_cast();
}
// (2)
template <class T>
auto any_cast(inplace_any_view &operand) -> T & {
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, U &>);
    if (auto *ptr = any_cast<U>(&operand)) {
        return static_cast<T>(*ptr);
    }
    throw bad_inplace_any_view_cast();
}
// (3)
template <class T>
auto any_cast(inplace_any_view &&operand) -> T { // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    using U = std::remove_cvref_t<T>;
    static_assert(std::is_constructible_v<T, U>);
    if (auto *ptr = any_cast<U>(&operand)) {
        return static_cast<T>(std::move(*ptr));
    }
    throw bad_inplace_any_view_cast();
}
// (4)
template <class T>
auto any_cast(const inplace_any_view *operand) noexcept -> const T * {
    static_assert(!std::is_void_v<T>);
    return operand && operand->type() == typeid(T)
               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
               ? reinterpret_cast<const T *>(operand->buffer_)
               : nullptr;
}
// (5)
template <class T>
auto any_cast(inplace_any_view *operand) noexcept -> T * {
    static_assert(!std::is_void_v<T>);
    return operand && operand->type() == typeid(T)
               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
               ? reinterpret_cast<T *>(operand->buffer_)
               : nullptr;
}

} // namespace mcpp