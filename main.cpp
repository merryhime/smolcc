// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <fmt/core.h>

#include "assert.h"
#include "lexer.h"
#include "parser.h"

static int iota() {
    static int value = 1;
    return value++;
}

template<typename T, typename U>
T* dyn(const U& e) {
    return dynamic_cast<T*>(e.get());
}

void emit_loc(const ExprPtr& expr) {
    fmt::print(".loc {} {} {}\n", expr->loc.file, expr->loc.line, expr->loc.col);
}

void emit_expr(const ExprPtr& expr) {
    if (auto e = dyn<IntegerConstantExpr>(expr)) {
        emit_loc(expr);
        fmt::print("movz x0, {}\n", e->value & 0xFFFF);
        if ((e->value >> 16) & 0xFFFF)
            fmt::print("movk x0, {}, lsl 16\n", (e->value >> 16) & 0xFFFF);
        if ((e->value >> 32) & 0xFFFF)
            fmt::print("movk x0, {}, lsl 32\n", (e->value >> 32) & 0xFFFF);
        if ((e->value >> 48) & 0xFFFF)
            fmt::print("movk x0, {}, lsl 48\n", (e->value >> 48) & 0xFFFF);
        return;
    }

    if (auto e = dyn<UnOpExpr>(expr)) {
        emit_expr(e->e);

        emit_loc(expr);
        switch (e->op) {
        case UnOpKind::Posate:
            // do nothing
            return;
        case UnOpKind::Negate:
            fmt::print("neg x0, x0\n");
            return;
        default:
            ASSERT(!"Unknown unop kind");
        }
    }

    if (auto e = dyn<BinOpExpr>(expr)) {
        emit_expr(e->lhs);
        fmt::print("str x0, [sp, -16]!\n");
        emit_expr(e->rhs);
        fmt::print("ldr x1, [sp], 16\n");

        emit_loc(expr);
        switch (e->op) {
        case BinOpKind::Add:
            fmt::print("add x0, x1, x0\n");
            return;
        case BinOpKind::Subtract:
            fmt::print("sub x0, x1, x0\n");
            return;
        case BinOpKind::Multiply:
            fmt::print("mul x0, x1, x0\n");
            return;
        case BinOpKind::Divide:
            fmt::print("udiv x0, x1, x0\n");  // unsigned divide for now
            return;
        case BinOpKind::Modulo:
            fmt::print("udiv x2, x1, x0\n");  // unsigned for now
            fmt::print("msub x0, x2, x0, x1\n");
            return;
        case BinOpKind::LessThan:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, lt\n");  // signed compare
            return;
        case BinOpKind::GreaterThan:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, gt\n");  // signed compare
            return;
        case BinOpKind::LessThanEqual:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, le\n");  // signed compare
            return;
        case BinOpKind::GreaterThanEqual:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, ge\n");  // signed compare
            return;
        case BinOpKind::Equal:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, eq\n");
            return;
        case BinOpKind::NotEqual:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, ne\n");
            return;
        case BinOpKind::BitAnd:
            fmt::print("and x0, x1, x0\n");
            return;
        case BinOpKind::BitXor:
            fmt::print("eor x0, x1, x0\n");
            return;
        case BinOpKind::BitOr:
            fmt::print("orr x0, x1, x0\n");
            return;
        default:
            ASSERT(!"Unknown binop kind");
        }
    }

    ASSERT(!"Unknown expr kind");
}

void emit_stmt(const StmtPtr& stmt) {
    if (auto s = dyn<CompoundStmt>(stmt)) {
        for (auto& i : s->items) {
            emit_stmt(i);
        }
        return;
    }

    if (auto s = dyn<ExprStmt>(stmt)) {
        if (s->e)
            emit_expr(s->e);
        return;
    }

    if (auto s = dyn<IfStmt>(stmt)) {
        const int i = iota();
        emit_expr(s->cond);
        fmt::print("cmp x0, 0\n");
        fmt::print("b.eq .if{}.else\n", i);
        emit_stmt(s->then_);
        fmt::print("b .if{}.end\n", i);
        fmt::print(".if{}.else:\n", i);
        if (s->else_)
            emit_stmt(s->else_);
        fmt::print(".if{}.end:\n", i);
        return;
    }

    if (auto s = dyn<LoopStmt>(stmt)) {
        const int i = iota();
        if (s->init) {
            emit_expr(s->init);
        }
        fmt::print(".loop{}.cond:\n", i);
        if (s->cond) {
            emit_expr(s->cond);
            fmt::print("cmp x0, 0\n");
            fmt::print("b.eq .loop{}.end\n", i);
        }
        emit_stmt(s->then);
        if (s->incr)
            emit_expr(s->incr);
        fmt::print("b .loop{}.cond\n", i);
        fmt::print(".loop{}.end:\n", i);
        return;
    }

    if (auto s = dyn<ReturnStmt>(stmt)) {
        if (s->e)
            emit_expr(s->e);
        fmt::print("ret\n");
        return;
    }

    ASSERT(!"Unknown stmt kind");
}

int main(int argc, char* argv[]) {
    ASSERT(argc == 2);

    Parser p{TokenStream{CharStream{1, argv[1]}}};

    fmt::print(".file 1 \"stdin\"\n");
    fmt::print(".text\n");
    fmt::print(".globl _main\n");
    fmt::print(".align 4\n");
    fmt::print("_main:\n");

    StmtPtr s = p.statement();
    emit_stmt(std::move(s));

    fmt::print("ret\n");

    return 0;
}
