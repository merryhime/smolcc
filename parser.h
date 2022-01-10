// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <memory>

#include "lexer.h"

struct Expr {
    explicit Expr(Location loc)
            : loc(loc) {}
    virtual ~Expr() {}

    Location loc;
};

using ExprPtr = std::unique_ptr<Expr>;

struct IntegerConstantExpr : public Expr {
    IntegerConstantExpr(Location loc, uintmax_t value)
            : value(value), Expr(loc) {}
    uintmax_t value;
};

enum class UnOpKind {
    Posate,
    Negate,
};

struct UnOpExpr : public Expr {
    UnOpExpr(Location loc, UnOpKind op, ExprPtr e)
            : op(op), e(std::move(e)), Expr(loc) {}

    UnOpKind op;
    ExprPtr e;
};

enum class BinOpKind {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    LShift,
    RShift,
    LessThan,
    GreaterThan,
    LessThanEqual,
    GreaterThanEqual,
    Equal,
    NotEqual,
    BitAnd,
    BitXor,
    BitOr,
    LogicalAnd,
    LogicalOr,
};

struct BinOpExpr : public Expr {
    BinOpExpr(Location loc, BinOpKind op, ExprPtr lhs, ExprPtr rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)), Expr(loc) {}

    BinOpKind op;
    ExprPtr lhs;
    ExprPtr rhs;
};

class Parser {
public:
    Parser(TokenStream inner)
            : inner(std::move(inner)) {}

    ExprPtr primary_expression();
    ExprPtr postfix_expression();
    ExprPtr unary_expression();
    ExprPtr cast_expression();
    ExprPtr multiplicative_expression();
    ExprPtr additive_expression();
    ExprPtr shift_expression();
    ExprPtr relational_expression();
    ExprPtr equality_expression();
    ExprPtr and_expression();
    ExprPtr exclusive_or_expression();
    ExprPtr inclusive_or_expression();
    ExprPtr logical_and_expression();
    ExprPtr logical_or_expression();
    ExprPtr conditional_expression();
    ExprPtr assignment_expression();
    ExprPtr expression();

private:
    TokenStream inner;
};
