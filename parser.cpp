// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#include "parser.h"

#include <memory>

#include "assert.h"

ExprPtr Parser::primary_expression() {
    if (inner.consume_if(PunctuatorKind::LParen)) {
        ExprPtr result = expression();
        ASSERT(inner.consume_if(PunctuatorKind::RParen));
        return result;
    }

    // TODO other types of primary expression

    const Token tok = inner.next();
    ASSERT(tok.kind == TokenKind::IntegerConstant);
    return std::make_unique<IntegerConstantExpr>(inner.loc(), tok.value);
}

ExprPtr Parser::postfix_expression() {
    // TODO
    return primary_expression();
}

ExprPtr Parser::unary_expression() {
    // TODO complete this
    if (inner.consume_if(PunctuatorKind::Plus)) {
        return std::make_unique<UnOpExpr>(inner.loc(), UnOpKind::Posate, cast_expression());
    }
    if (inner.consume_if(PunctuatorKind::Minus)) {
        return std::make_unique<UnOpExpr>(inner.loc(), UnOpKind::Negate, cast_expression());
    }
    return postfix_expression();
}

ExprPtr Parser::cast_expression() {
    // TODO
    return unary_expression();
}

ExprPtr Parser::multiplicative_expression() {
    ExprPtr e = cast_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::Star)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::Multiply, std::move(e), cast_expression());
        } else if (inner.consume_if(PunctuatorKind::Slash)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::Divide, std::move(e), cast_expression());
        } else if (inner.consume_if(PunctuatorKind::Modulo)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::Modulo, std::move(e), cast_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::additive_expression() {
    ExprPtr e = multiplicative_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::Plus)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::Add, std::move(e), multiplicative_expression());
        } else if (inner.consume_if(PunctuatorKind::Minus)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::Subtract, std::move(e), multiplicative_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::shift_expression() {
    ExprPtr e = additive_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::LLAngle)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::LShift, std::move(e), additive_expression());
        } else if (inner.consume_if(PunctuatorKind::RRAngle)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::RShift, std::move(e), additive_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::relational_expression() {
    ExprPtr e = shift_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::LAngle)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::LessThan, std::move(e), shift_expression());
        } else if (inner.consume_if(PunctuatorKind::RAngle)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::GreaterThan, std::move(e), shift_expression());
        } else if (inner.consume_if(PunctuatorKind::LAngleEq)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::LessThanEqual, std::move(e), shift_expression());
        } else if (inner.consume_if(PunctuatorKind::RAngleEq)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::GreaterThanEqual, std::move(e), shift_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::equality_expression() {
    ExprPtr e = relational_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::EqEq)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::Equal, std::move(e), relational_expression());
        } else if (inner.consume_if(PunctuatorKind::NotEq)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::NotEqual, std::move(e), relational_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::and_expression() {
    ExprPtr e = equality_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::And)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::BitAnd, std::move(e), equality_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::exclusive_or_expression() {
    ExprPtr e = and_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::Caret)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::BitXor, std::move(e), and_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::inclusive_or_expression() {
    ExprPtr e = exclusive_or_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::Or)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::BitOr, std::move(e), exclusive_or_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::logical_and_expression() {
    ExprPtr e = inclusive_or_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::AndAnd)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::LogicalAnd, std::move(e), inclusive_or_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::logical_or_expression() {
    ExprPtr e = logical_and_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::OrOr)) {
            e = std::make_unique<BinOpExpr>(inner.loc(), BinOpKind::LogicalOr, std::move(e), logical_and_expression());
        } else {
            return e;
        }
    }
}

ExprPtr Parser::conditional_expression() {
    // TODO
    return logical_or_expression();
}

ExprPtr Parser::assignment_expression() {
    // TODO
    return conditional_expression();
}

ExprPtr Parser::expression() {
    // TODO comma operator
    return assignment_expression();
}

StmtPtr Parser::compound_statement() {
    std::vector<StmtPtr> items;

    ASSERT(inner.consume_if(PunctuatorKind::LBrace));
    Location loc = inner.loc();

    while (!inner.consume_if(PunctuatorKind::RBrace)) {
        items.emplace_back(statement());
    }
    return std::make_unique<CompoundStmt>(loc, std::move(items));
}

StmtPtr Parser::expression_statement() {
    ExprPtr e = expression();
    ASSERT(inner.consume_if(PunctuatorKind::Semi));

    return std::make_unique<ExprStmt>(e->loc, std::move(e));
}

StmtPtr Parser::if_statement() {
    ASSERT(inner.consume_if_identifier("if"));
    const Location loc = inner.loc();

    ASSERT(inner.consume_if(PunctuatorKind::LParen));
    ExprPtr e = expression();
    ASSERT(inner.consume_if(PunctuatorKind::RParen));

    StmtPtr then_ = statement();

    if (inner.consume_if_identifier("else")) {
        StmtPtr else_ = statement();

        return std::make_unique<IfStmt>(loc, std::move(e), std::move(then_), std::move(else_));
    }
    return std::make_unique<IfStmt>(loc, std::move(e), std::move(then_), nullptr);
}

StmtPtr Parser::for_statement() {
    ASSERT(inner.consume_if_identifier("for"));
    const Location loc = inner.loc();
    ExprPtr init, cond, incr;

    ASSERT(inner.consume_if(PunctuatorKind::LParen));
    if (!inner.consume_if(PunctuatorKind::Semi)) {
        init = expression();
        ASSERT(inner.consume_if(PunctuatorKind::Semi));
    }
    if (!inner.consume_if(PunctuatorKind::Semi)) {
        cond = expression();
        ASSERT(inner.consume_if(PunctuatorKind::Semi));
    }
    if (!inner.consume_if(PunctuatorKind::RParen)) {
        incr = expression();
    }
    ASSERT(inner.consume_if(PunctuatorKind::RParen));

    StmtPtr then = statement();
    return std::make_unique<ForStmt>(loc, std::move(init), std::move(cond), std::move(incr), std::move(then));
}

StmtPtr Parser::return_statement() {
    ASSERT(inner.consume_if_identifier("return"));
    const Location loc = inner.loc();

    if (inner.peek(PunctuatorKind::Semi)) {
        return std::make_unique<ReturnStmt>(loc, nullptr);
    }

    ExprPtr e = expression();
    ASSERT(inner.consume_if(PunctuatorKind::Semi));

    return std::make_unique<ReturnStmt>(loc, std::move(e));
}

StmtPtr Parser::statement() {
    // TODO
    if (inner.peek(PunctuatorKind::Semi)) {
        // null statement
        inner.next();
        return std::make_unique<ExprStmt>(inner.loc(), nullptr);
    }
    if (inner.peek(PunctuatorKind::LBrace)) {
        return compound_statement();
    }
    if (inner.peek_identifier("if")) {
        return if_statement();
    }
    if (inner.peek_identifier("for")) {
        return for_statement();
    }
    if (inner.peek_identifier("return")) {
        return return_statement();
    }
    return expression_statement();
}
