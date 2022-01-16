// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <type_traits>

namespace detail {

template<class T>
struct cloner {
    virtual ~cloner() = default;
    virtual T* clone(T* value) const = 0;
    virtual std::unique_ptr<cloner<T>> clone() const = 0;
};

template<class T, class U = T>
struct direct_cloner : public cloner<T> {
    T* clone(T* value) const override {
        return new U(*static_cast<U*>(value));
    }
    std::unique_ptr<cloner<T>> clone() const override {
        return std::make_unique<direct_cloner<T, U>>();
    }
};

template<class T, class U>
struct delegating_cloner : public cloner<T> {
    delegating_cloner(std::unique_ptr<cloner<U>> inner)
            : inner(std::move(inner)) {}

    std::unique_ptr<cloner<U>> inner;
    T* clone(T* value) const override {
        return inner->clone(static_cast<U*>(value));
    }
    std::unique_ptr<cloner<T>> clone() const override {
        return std::make_unique<delegating_cloner<T, U>>(inner->clone());
    }
};

}  // namespace detail

template<class T>
class poly_value {
    static_assert(!std::is_union_v<T> && std::is_class_v<T>);

    template<class>
    friend class poly_value;
    template<class T_, class U, class... Ts>
    friend poly_value<T_> make_poly_value(Ts&&... ts);

    std::unique_ptr<T> ptr;
    std::unique_ptr<detail::cloner<T>> cloner;

public:
    using element_type = T;

    constexpr poly_value() noexcept = default;
    constexpr poly_value(std::nullptr_t) noexcept {}

    poly_value(const poly_value& p) {
        if (!p)
            return;
        ptr = std::unique_ptr<T>(p.cloner->clone(p.ptr.get()));
        cloner = p.cloner->clone();
    }

    poly_value(poly_value&& p) noexcept {
        ptr = std::move(p.ptr);
        cloner = std::move(p.cloner);
    }

    template<class U, class V = std::enable_if_t<!std::is_same_v<T, U> && std::is_convertible_v<U*, T*>>>
    poly_value(const poly_value<U>& p) {
        ptr = std::unique_ptr<T>(p.cloner->clone(p.ptr.get()));
        cloner = std::make_unique<detail::delegating_cloner<T, U>>(p.cloner->clone());
    }

    template<class U, class V = std::enable_if_t<!std::is_same_v<T, U> && std::is_convertible_v<U*, T*>>>
    poly_value(poly_value<U>&& p) {
        ptr = std::move(p.ptr);
        cloner = std::make_unique<detail::delegating_cloner<T, U>>(std::move(p.cloner));
    }

    ~poly_value() = default;

    poly_value& operator=(const poly_value& p) {
        if (std::addressof(p) == this)
            return *this;

        if (!p) {
            ptr.reset();
            cloner.reset();
            return *this;
        }

        ptr = std::unique_ptr<T>(p.cloner->clone(p.ptr.get()));
        cloner = p.cloner->clone();
        return *this;
    }

    poly_value& operator=(poly_value&& p) noexcept {
        if (std::addressof(p) == this)
            return *this;

        ptr = std::move(p.ptr);
        cloner = std::move(p.cloner);
        return *this;
    }

    void swap(poly_value& p) noexcept {
        using std::swap;
        swap(ptr, p.ptr);
        swap(cloner, p.cloner);
    }

    const T& operator*() const { return *ptr; }
    T& operator*() { return *ptr; }
    const T* operator->() const { return ptr.get(); }
    T* operator->() { return ptr.get(); }
    explicit operator bool() const noexcept { return static_cast<bool>(ptr); }

    template<typename U>
    U* cast() const { return dynamic_cast<U*>(ptr.get()); }
};

template<class T, class U = T, class... Ts>
poly_value<T> make_poly_value(Ts&&... ts) {
    poly_value<T> result;
    result.cloner = std::make_unique<detail::direct_cloner<T, U>>();
    result.ptr = std::make_unique<U>(std::forward<Ts>(ts)...);
    return result;
}

template<class T>
void swap(poly_value<T>& p, poly_value<T>& u) noexcept {
    p.swap(u);
}
