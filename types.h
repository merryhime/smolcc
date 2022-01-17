// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <memory>

struct Type {
    virtual ~Type() = default;
    virtual bool is_pointer() const { return false; }
    virtual std::size_t size() const { return 8; }
};

using TypeVal = poly_value<Type>;

template<typename T, typename... Ts>
inline TypeVal make_type(Ts&&... ts) {
    return make_poly_value<Type, T>(std::forward<Ts>(ts)...);
}

struct InvalidType : public Type {};

inline TypeVal make_invalid_type() {
    return make_type<InvalidType>();
}

enum PrimitiveTypeKind {
    Int
};

struct PrimitiveType : public Type {
    explicit PrimitiveType(PrimitiveTypeKind kind)
            : kind(kind) {}

    PrimitiveTypeKind kind;
};

inline TypeVal make_int_type() {
    return make_type<PrimitiveType>(PrimitiveTypeKind::Int);
}

struct PointerType : public Type {
    explicit PointerType(TypeVal base)
            : base(std::move(base)) {}

    TypeVal base;

    bool is_pointer() const override { return true; }
};

inline TypeVal make_ptr_type(TypeVal inner) {
    return make_type<PointerType>(std::move(inner));
}
