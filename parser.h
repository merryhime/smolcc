// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "lexer.h"
#include "poly_value.h"
#include "types.h"

struct Expr {
    explicit Expr(Location loc)
            : loc(loc) {}
    virtual ~Expr() {}

    Location loc;
    virtual TypeVal type() = 0;
};

using ExprVal = poly_value<Expr>;

template<typename T, typename... Ts>
inline ExprVal make_expr(Ts&&... ts) {
    return make_poly_value<Expr, T>(std::forward<Ts>(ts)...);
}

struct IntegerConstantExpr : public Expr {
    IntegerConstantExpr(Location loc, uintmax_t value)
            : value(value), Expr(loc) {}
    uintmax_t value;

    TypeVal type() override { return make_int_type(); }
};

struct VariableExpr : public Expr {
    VariableExpr(Location loc, std::string ident)
            : ident(ident), Expr(loc) {}
    std::string ident;

    TypeVal type() override { return make_int_type(); }
};

enum class UnOpKind {
    AddressOf,
    Dereference,
    Posate,
    Negate,
};

struct UnOpExpr : public Expr {
    UnOpExpr(Location loc, UnOpKind op, ExprVal e)
            : op(op), e(std::move(e)), Expr(loc) {}

    UnOpKind op;
    ExprVal e;

    TypeVal type() override {
        TypeVal et = e->type();
        switch (op) {
        case UnOpKind::AddressOf:
            return make_ptr_type(std::move(et));
        case UnOpKind::Dereference:
            return et->is_pointer() ? et.cast<PointerType>()->base : make_int_type();
        case UnOpKind::Posate:
        case UnOpKind::Negate:
            return et;
        }
    }
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
    BinOpExpr(Location loc, BinOpKind op, ExprVal lhs, ExprVal rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)), Expr(loc) {}

    BinOpKind op;
    ExprVal lhs;
    ExprVal rhs;

    TypeVal type() override {
        const TypeVal lt = lhs->type();
        const TypeVal rt = rhs->type();
        switch (op) {
        case BinOpKind::Add:
            if (lt->is_pointer() && rt->is_pointer())
                return make_invalid_type();
            if (rt->is_pointer())
                return rt;
            return lt;
        case BinOpKind::Subtract:
            if (lt->is_pointer() && rt->is_pointer())
                return make_int_type();
            if (rt->is_pointer())
                return make_invalid_type();
            return lt;
        case BinOpKind::Multiply:
        case BinOpKind::Divide:
        case BinOpKind::Modulo:
        case BinOpKind::LShift:
        case BinOpKind::RShift:
        case BinOpKind::BitAnd:
        case BinOpKind::BitXor:
        case BinOpKind::BitOr:
            return lt;
        case BinOpKind::LessThan:
        case BinOpKind::GreaterThan:
        case BinOpKind::LessThanEqual:
        case BinOpKind::GreaterThanEqual:
        case BinOpKind::Equal:
        case BinOpKind::NotEqual:
        case BinOpKind::LogicalAnd:
        case BinOpKind::LogicalOr:
            return make_int_type();
        }
    }
};

struct AssignExpr : public Expr {
    AssignExpr(Location loc, ExprVal lhs, ExprVal rhs)
            : lhs(std::move(lhs)), rhs(std::move(rhs)), Expr(loc) {}

    ExprVal lhs;
    ExprVal rhs;

    TypeVal type() override { return lhs->type(); }
};

struct Stmt {
    explicit Stmt(Location loc)
            : loc(loc) {}
    virtual ~Stmt() {}

    Location loc;
};

using StmtVal = poly_value<Stmt>;

template<typename T, typename... Ts>
inline StmtVal make_stmt(Ts&&... ts) {
    return make_poly_value<Stmt, T>(std::forward<Ts>(ts)...);
}

struct CompoundStmt : public Stmt {
    // TODO: block-item should be variant<Stmt, Decl>
    CompoundStmt(Location loc, std::vector<StmtVal> items)
            : items(std::move(items)), Stmt(loc) {}

    std::vector<StmtVal> items;
};

struct ExprStmt : public Stmt {
    ExprStmt(Location loc, ExprVal e)
            : e(std::move(e)), Stmt(loc) {}

    ExprVal e;
};

struct IfStmt : public Stmt {
    IfStmt(Location loc, ExprVal cond, StmtVal then_, StmtVal else_)
            : cond(std::move(cond)), then_(std::move(then_)), else_(std::move(else_)), Stmt(loc) {}

    ExprVal cond;
    StmtVal then_;
    StmtVal else_;
};

struct LoopStmt : public Stmt {
    LoopStmt(Location loc, ExprVal init, ExprVal cond, ExprVal incr, StmtVal then)
            : init(std::move(init)), cond(std::move(cond)), incr(std::move(incr)), then(std::move(then)), Stmt(loc) {}

    ExprVal init, cond, incr;
    StmtVal then;
};

struct ReturnStmt : public Stmt {
    ReturnStmt(Location loc, ExprVal e)
            : e(std::move(e)), Stmt(loc) {}

    ExprVal e;
};

// TODO: Temporary
struct DeclStmt : public Stmt {
    DeclStmt(Location loc, std::string ident)
            : ident(ident), Stmt(loc) {}

    std::string ident;
};

class Parser {
public:
    Parser(TokenStream inner)
            : inner(std::move(inner)) {}

    ExprVal primary_expression();
    ExprVal postfix_expression();
    ExprVal unary_expression();
    ExprVal cast_expression();
    ExprVal multiplicative_expression();
    ExprVal additive_expression();
    ExprVal shift_expression();
    ExprVal relational_expression();
    ExprVal equality_expression();
    ExprVal and_expression();
    ExprVal exclusive_or_expression();
    ExprVal inclusive_or_expression();
    ExprVal logical_and_expression();
    ExprVal logical_or_expression();
    ExprVal conditional_expression();
    ExprVal assignment_expression();
    ExprVal expression();

    StmtVal compound_statement();
    StmtVal expression_statement();
    StmtVal if_statement();
    StmtVal while_statement();
    StmtVal for_statement();
    StmtVal return_statement();
    StmtVal statement();

    StmtVal declaration();

private:
    TokenStream inner;
};
