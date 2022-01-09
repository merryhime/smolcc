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

template<typename T>
T* dyn(const ExprPtr& e) {
    return dynamic_cast<T*>(e.get());
}

void emit_expr(const ExprPtr& expr) {
    if (auto e = dyn<IntegerConstantExpr>(expr)) {
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

int main(int argc, char* argv[]) {
    ASSERT(argc == 2);

    Parser p{TokenStream{CharStream{0, argv[1]}}};

    fmt::print(".text\n");
    fmt::print(".globl _main\n");
    fmt::print(".align 4\n");
    fmt::print("_main:\n");

    ExprPtr e = p.expression();
    emit_expr(std::move(e));

    fmt::print("ret\n");

    return 0;
}
