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
constexpr inline std::size_t small_buffer_size = 3 * sizeof(void *);
constexpr inline std::size_t small_buffer_alignment = std::alignment_of_v<void *>;
using small_buffer = std::aligned_storage_t<small_buffer_size, small_buffer_alignment>;

template <typename T>
constexpr inline bool is_small_object_v = sizeof(T) <= sizeof(small_buffer) &&                               //
                                          std::alignment_of_v<small_buffer> % std::alignment_of_v<T> == 0 && //
                                          std::is_nothrow_move_constructible_v<T>;

enum class action { destroy, move, get, typeinfo };

template <class T>
struct small_buffer_handler;
template <class T>
struct default_handler;

template <class T>
using handler = std::conditional_t<is_small_object_v<T>, small_buffer_handler<T>, default_handler<T>>;

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
    constexpr unique_any() noexcept = default;
    // https://en.cppreference.com/w/cpp/utility/any/any (2)
    unique_any(const unique_any &other) = delete;
    // https://en.cppreference.com/w/cpp/utility/any/any (3)
    unique_any(unique_any &&other) noexcept {
        if (other.h_ != nullptr) {
            other.call(action::move, this);
        }
    }
    // https://en.cppreference.com/w/cpp/utility/any/any (4)
    template <class ValueType, std::enable_if_t<!std::is_same_v<std::decay_t<ValueType>, unique_any> &&
                                                !detail::is_in_place_type_v<std::decay_t<ValueType>>> * = nullptr>
    unique_any(ValueType &&value) {
        detail::handler<std::decay_t<ValueType>>::create(*this, std::forward<ValueType>(value));
    }
    // https://en.cppreference.com/w/cpp/utility/any/any (5)
    template <class ValueType, class... Args,
              std::enable_if_t<std::is_constructible_v<std::decay_t<ValueType>, Args...>> * = nullptr>
    explicit unique_any(std::in_place_type_t<ValueType> /*unused*/, Args &&...args) {
        detail::handler<std::decay_t<ValueType>>::create(*this, std::forward<Args>(args)...);
    }
    // https://en.cppreference.com/w/cpp/utility/any/any (6)
    template <class ValueType, class U, class... Args,
              std::enable_if_t<std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...>>
                  * = nullptr>
    explicit unique_any(std::in_place_type_t<ValueType> /*unused*/, std::initializer_list<U> il, Args &&...args) {
        detail::handler<std::decay_t<ValueType>>::create(*this, il, std::forward<Args>(args)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Assignment operators
    // https://en.cppreference.com/w/cpp/utility/any/operator%3D (1)
    auto operator=(const unique_any &rhs) -> unique_any & = delete;
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
    ~unique_any() { this->reset(); }

    ///////////////////////////////////////////////////////////////////////////
    // Modifiers
    // https://en.cppreference.com/w/cpp/utility/any/emplace (1)
    template <class ValueType, class... Args,
              std::enable_if_t<std::is_constructible_v<std::decay_t<ValueType>, Args...>> * = nullptr>
    auto emplace(Args &&...args) -> std::decay_t<ValueType> & {
        reset();
        return detail::handler<std::decay_t<ValueType>>::create(*this, std::forward<Args>(args)...);
    }
    // https://en.cppreference.com/w/cpp/utility/any/emplace (2)
    template <class ValueType, class U, class... Args,
              std::enable_if_t<std::is_constructible_v<std::decay_t<ValueType>, std::initializer_list<U> &, Args...>>
                  * = nullptr>
    auto emplace(std::initializer_list<U> il, Args &&...args) -> std::decay_t<ValueType> & {
        reset();
        return detail::handler<std::decay_t<ValueType>>::create(*this, il, std::forward<Args>(args)...);
    }
    // https://en.cppreference.com/w/cpp/utility/any/reset
    void reset() noexcept {
        if (h_ != nullptr) {
            this->call(action::destroy);
        }
    }
    // https://en.cppreference.com/w/cpp/utility/any/swap
    void swap(unique_any &other) noexcept {
        if (this == &other) {
            return;
        }
        if (h_ != nullptr && other.h_ != nullptr) {
            auto tmp = unique_any();
            other.call(action::move, &tmp);
            this->call(action::move, &other);
            tmp.call(action::move, this);
        } else if (h_ != nullptr) {
            this->call(action::move, &other);
        } else if (other.h_ != nullptr) {
            other.call(action::move, this);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Observers
    // https://en.cppreference.com/w/cpp/utility/any/has_value
    [[nodiscard]] auto has_value() const noexcept -> bool { return h_ != nullptr; }
    // https://en.cppreference.com/w/cpp/utility/any/type
    [[nodiscard]] auto type() const noexcept -> const std::type_info & {
        return has_value() ? *static_cast<std::type_info const *>(h_(action::typeinfo, nullptr, nullptr))
                           : typeid(void);
    }

  private:
    using action = detail::action;
    using handle_func = void *(*)(action, unique_any *, unique_any *);

    union storage {
        constexpr storage() : ptr(nullptr) {}
        void *ptr;
        detail::small_buffer buf;
    };

    auto call(action a, unique_any *other = nullptr) -> void * { return h_(a, this, other); }

    template <typename T>
    auto unsafe_cast() -> T * {
        return static_cast<T *>(call(action::get));
    }

    template <typename T>
    auto unsafe_cast() const -> const T * {
        return static_cast<const T *>(const_cast<unique_any *>(this)->call(action::get));
    }

    template <class>
    friend struct detail::small_buffer_handler;

    template <class>
    friend struct detail::default_handler;

    template <typename T>
    friend auto any_cast(const unique_any *operand) noexcept -> const T *;

    template <typename T>
    friend auto any_cast(unique_any *operand) noexcept -> T *;

    handle_func h_ = nullptr;
    storage s_;
};

namespace detail {
template <class T>
struct small_buffer_handler {
    static auto handle(action act, unique_any *self, unique_any *other) -> void * {
        switch (act) {
            case action::destroy:
                destroy(*self);
                return nullptr;
            case action::move:
                move(*self, *other);
                return nullptr;
            case action::get:
                return get(*self);
            case action::typeinfo:
                return type_info();
        }
    }

    template <class... Args>
    static auto create(unique_any &dest, Args &&...args) -> T & {
        using allocator = std::allocator<T>;
        using allocator_traits = std::allocator_traits<allocator>;
        allocator a;
        T *ret = static_cast<T *>(static_cast<void *>(&dest.s_.buf));
        allocator_traits::construct(a, ret, std::forward<Args>(args)...);
        dest.h_ = &handle;
        return *ret;
    }

  private:
    static void destroy(unique_any &self) {
        using allocator = std::allocator<T>;
        using allocator_traits = std::allocator_traits<allocator>;
        allocator a;
        T *p = static_cast<T *>(static_cast<void *>(&self.s_.buf));
        allocator_traits::destroy(a, p);
        self.h_ = nullptr;
    }
    static void move(unique_any &self, unique_any &dest) {
        create(dest, std::move(*static_cast<T *>(static_cast<void *>(&self.s_.buf))));
        destroy(self);
    }
    static auto get(unique_any &self) -> void * { return static_cast<void *>(&self.s_.buf); }
    static auto type_info() -> void * { return const_cast<void *>(static_cast<void const *>(&typeid(T))); }
};

template <typename Allocator, typename std::allocator_traits<Allocator>::size_type size>
struct allocator_deleter {
    void operator()(typename std::allocator_traits<Allocator>::pointer p) noexcept {
        std::allocator_traits<Allocator>::deallocate(Allocator{}, p, size);
    }
};

template <class T>
struct default_handler {
    static auto handle(action act, unique_any *self, unique_any *other) -> void * {
        switch (act) {
            case action::destroy:
                destroy(*self);
                return nullptr;
            case action::move:
                move(*self, *other);
                return nullptr;
            case action::get:
                return get(*self);
            case action::typeinfo:
                return type_info();
        }
    }

    template <class... Args>
    static auto create(unique_any &dest, Args &&...args) -> T & {
        using allocator = std::allocator<T>;
        using allocator_traits = std::allocator_traits<allocator>;
        using deleter = allocator_deleter<allocator, 1>;
        auto alloc = allocator{};
        auto holder = std::unique_ptr<T, deleter>(allocator_traits::allocate(alloc, 1));
        auto *ptr = holder.get();
        allocator_traits::construct(alloc, ptr, std::forward<Args>(args)...);
        dest.s_.ptr = holder.release();
        dest.h_ = &handle;
        return *ptr;
    }

  private:
    static void destroy(unique_any &self) {
        using allocator = std::allocator<T>;
        using allocator_traits = std::allocator_traits<allocator>;
        auto alloc = allocator{};
        T *ptr = static_cast<T *>(self.s_.ptr);
        allocator_traits::destroy(alloc, ptr);
        allocator_traits::deallocate(alloc, ptr, 1);
        self.h_ = nullptr;
    }
    static void move(unique_any &self, unique_any &dest) {
        dest.s_.ptr = self.s_.ptr;
        dest.h_ = &handle;
        self.h_ = nullptr;
    }
    static auto get(unique_any &self) -> void * { return self.s_.ptr; }
    static auto type_info() -> void * { return const_cast<void *>(static_cast<void const *>(&typeid(T))); }
};

} // namespace detail

// https://en.cppreference.com/w/cpp/utility/any/swap2
inline void swap(unique_any &lhs, unique_any &rhs) noexcept {
    lhs.swap(rhs);
}

// https://en.cppreference.com/w/cpp/utility/any/any_cast (1)
template <class T>
auto any_cast(const unique_any &operand) -> T {
    using U = std::remove_cv_t<std::remove_reference_t<T>>;
    static_assert(std::is_constructible_v<T, const U &>);
    if (auto ptr = any_cast<std::add_const_t<U>>(&operand)) {
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
    static_assert(!std::is_reference_v<T>);
    if (operand && operand->type() == typeid(T)) {
        return operand->unsafe_cast<T>();
    }
    return nullptr;
}

// https://en.cppreference.com/w/cpp/utility/any/any_cast (5)
template <class T>
auto any_cast(unique_any *operand) noexcept -> T * {
    static_assert(!std::is_reference_v<T>);
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