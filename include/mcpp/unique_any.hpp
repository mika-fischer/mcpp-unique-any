// Copyright Mika Fischer 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <any>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace mcpp {

namespace detail {
template <typename T>
struct is_in_place_type : std::false_type {};
template <typename T>
struct is_in_place_type<std::in_place_type_t<T>> : std::true_type {};
template <typename T>
inline constexpr bool is_in_place_type_v = is_in_place_type<T>::value;
} // namespace detail

class unique_any {
  public:
    ///////////////////////////////////////////////////////////////////////////
    // Constructors
    // https://en.cppreference.com/w/cpp/utility/any/any (1)
    constexpr unique_any() = default;
    // https://en.cppreference.com/w/cpp/utility/any/any (2)
    unique_any(const unique_any &other) = delete;
    // https://en.cppreference.com/w/cpp/utility/any/any (3)
    unique_any(unique_any &&other) noexcept { std::swap(other.state_, state_); }
    // https://en.cppreference.com/w/cpp/utility/any/any (4)
    template <class ValueType, std::enable_if_t<!std::is_same_v<std::decay_t<ValueType>, unique_any> &&
                                                !detail::is_in_place_type_v<std::decay_t<ValueType>>> * = nullptr>
    unique_any(ValueType &&value) : state_(new model<std::decay_t<ValueType>>(std::forward<ValueType>(value))) {}
    // https://en.cppreference.com/w/cpp/utility/any/any (5)
    template <class ValueType, class... Args,
              std::enable_if_t<std::is_constructible_v<std::decay_t<ValueType>, Args...>> * = nullptr>
    explicit unique_any(std::in_place_type_t<ValueType> /*unused*/, Args &&...args)
        : state_(new model<std::decay_t<ValueType>>(std::forward<Args>(args)...)) {}
    // https://en.cppreference.com/w/cpp/utility/any/any (6)
    template <class ValueType, class U, class... Args,
              std::enable_if_t<std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...>>
                  * = nullptr>
    explicit unique_any(std::in_place_type_t<ValueType> /*unused*/, std::initializer_list<U> il, Args &&...args)
        : state_(new model<std::decay_t<ValueType>>(il, std::forward<Args>(args)...)) {}

    ///////////////////////////////////////////////////////////////////////////
    // Assignment operators
    // https://en.cppreference.com/w/cpp/utility/any/operator%3D (1)
    auto operator=(const unique_any &rhs) = delete;
    // https://en.cppreference.com/w/cpp/utility/any/operator%3D (2)
    auto operator=(unique_any &&rhs) noexcept -> unique_any & {
        unique_any(std::move(rhs)).swap(*this);
        return *this;
    }
    // https://en.cppreference.com/w/cpp/utility/any/operator%3D (3)
    template <typename ValueType, std::enable_if_t<!std::is_same_v<std::decay_t<ValueType>, unique_any>> * = nullptr>
    auto operator=(ValueType &&rhs) -> unique_any & {
        unique_any(std::forward<ValueType>(rhs)).swap(*this);
        return *this;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Destructor
    // https://en.cppreference.com/w/cpp/utility/any/~any
    ~unique_any() = default;

    ///////////////////////////////////////////////////////////////////////////
    // Modifiers
    // https://en.cppreference.com/w/cpp/utility/any/emplace (1)
    template <class ValueType, class... Args,
              std::enable_if_t<std::is_constructible_v<std::decay_t<ValueType>, Args...>> * = nullptr>
    auto emplace(Args &&...args) -> std::decay_t<ValueType> & {
        state_.reset(new model<std::decay_t<ValueType>>(std::forward<Args>(args)...));
    }
    // https://en.cppreference.com/w/cpp/utility/any/emplace (2)
    template <class ValueType, class U, class... Args,
              std::enable_if_t<std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...>>
                  * = nullptr>
    auto emplace(std::initializer_list<U> il, Args &&...args) -> std::decay_t<ValueType> & {
        state_.reset(new model<std::decay_t<ValueType>>(il, std::forward<Args>(args)...));
    }
    // https://en.cppreference.com/w/cpp/utility/any/reset
    void reset() noexcept { state_.reset(); }
    // https://en.cppreference.com/w/cpp/utility/any/swap
    void swap(unique_any &other) noexcept { std::swap(other.state_, state_); }

    ///////////////////////////////////////////////////////////////////////////
    // Observers
    // https://en.cppreference.com/w/cpp/utility/any/has_value
    [[nodiscard]] auto has_value() const noexcept -> bool { return state_ != nullptr; }
    // https://en.cppreference.com/w/cpp/utility/any/type
    [[nodiscard]] auto type() const noexcept -> const std::type_info & {
        return has_value() ? state_->type() : typeid(void);
    }

  private:
    struct interface {
        virtual ~interface() = default;
        [[nodiscard]] virtual auto type() -> const std::type_info & = 0;
        [[nodiscard]] virtual auto pointer() -> void * = 0;
        [[nodiscard]] virtual auto pointer() const -> const void * = 0;
    };

    template <typename T>
    struct model : interface {
        T value;
        template <typename... Args>
        explicit model(Args &&...args) : value(std::forward<Args>(args)...) {}
        [[nodiscard]] auto type() -> const std::type_info & override { return typeid(T); }
        [[nodiscard]] auto pointer() -> void * override { return &value; }
        [[nodiscard]] auto pointer() const -> const void * override { return &value; }
    };

    template <typename T>
    auto unsafe_cast() -> T * {
        return static_cast<T *>(state_->pointer());
    }

    template <typename T>
    auto unsafe_cast() const -> const T * {
        return static_cast<const T *>(state_->pointer());
    }

    template <typename T>
    friend auto any_cast(const unique_any *operand) noexcept -> const T *;

    template <typename T>
    friend auto any_cast(unique_any *operand) noexcept -> T *;

    std::unique_ptr<interface> state_;
};

// https://en.cppreference.com/w/cpp/utility/any/any_cast (1)
template <class T>
auto any_cast(const unique_any &operand) -> T {
    using U = std::remove_cv_t<std::remove_reference_t<T>>;
    static_assert(std::is_constructible_v<T, const U &>);
    if (auto ptr = any_cast<U>(&operand)) {
        return static_cast<T>(*ptr);
    }
    throw std::bad_any_cast();
}

// https://en.cppreference.com/w/cpp/utility/any/any_cast (2)
template <class T>
auto any_cast(unique_any &operand) -> T {
    using U = std::remove_cv_t<std::remove_reference_t<T>>;
    static_assert(std::is_constructible_v<T, U &>);
    if (auto ptr = any_cast<U>(&operand)) {
        return static_cast<T>(*ptr);
    }
    throw std::bad_any_cast();
}

// https://en.cppreference.com/w/cpp/utility/any/any_cast (3)
template <class T>
auto any_cast(unique_any &&operand) -> T {
    using U = std::remove_cv_t<std::remove_reference_t<T>>;
    static_assert(std::is_constructible_v<T, U>);
    if (auto ptr = any_cast<U>(&operand)) {
        return static_cast<T>(std::move(*ptr));
    }
    throw std::bad_any_cast();
}

// https://en.cppreference.com/w/cpp/utility/any/any_cast (4)
template <class T>
auto any_cast(const unique_any *operand) noexcept -> const T * {
    if (operand && operand->type() == typeid(T)) {
        return operand->unsafe_cast<T>();
    }
    return nullptr;
}

// https://en.cppreference.com/w/cpp/utility/any/any_cast (5)
template <class T>
auto any_cast(unique_any *operand) noexcept -> T * {
    if (operand && operand->type() == typeid(T)) {
        return operand->unsafe_cast<T>();
    }
    return nullptr;
}

// https://en.cppreference.com/w/cpp/utility/any/make_any (1)
template <class T, class... Args>
auto make_unique_any(Args &&...args) -> unique_any {
    return unique_any(std::in_place_type<T>, std::forward<Args>(args)...);
}

// https://en.cppreference.com/w/cpp/utility/any/make_any (2)
template <class T, class U, class... Args>
auto make_unique_any(std::initializer_list<U> il, Args &&...args) -> unique_any {
    return unique_any(std::in_place_type<T>, il, std::forward<Args>(args)...);
}

} // namespace mcpp