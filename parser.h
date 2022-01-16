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
    // TypePtr type;
};

using ExprVal = poly_value<Expr>;

struct IntegerConstantExpr : public Expr {
    IntegerConstantExpr(Location loc, uintmax_t value)
            : value(value), Expr(loc) {}
    uintmax_t value;
};

struct VariableExpr : public Expr {
    VariableExpr(Location loc, std::string ident)
            : ident(ident), Expr(loc) {}
    std::string ident;
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
};

struct AssignExpr : public Expr {
    AssignExpr(Location loc, ExprVal lhs, ExprVal rhs)
            : lhs(std::move(lhs)), rhs(std::move(rhs)), Expr(loc) {}

    ExprVal lhs;
    ExprVal rhs;
};

struct Stmt {
    explicit Stmt(Location loc)
            : loc(loc) {}
    virtual ~Stmt() {}

    Location loc;
};

using StmtVal = poly_value<Stmt>;

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
