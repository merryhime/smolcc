// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#include "lexer.h"

#include <optional>

#include "assert.h"

static bool isspace(std::optional<char> ch) {
    return ch == ' ' || ch == '\t' || ch == '\v' || ch == '\r' || ch == '\n';
}

static bool isdecimaldigit(std::optional<char> ch) {
    return ch && (*ch >= '0' && *ch <= '9');
}

static bool isidentifiernondigit(std::optional<char> ch) {
    return ch && (*ch == '_' || (*ch >= 'a' && *ch <= 'z') || (*ch >= 'A' && *ch <= 'Z'));
}

Token TokenStream::tok() {
    while (inner.peek()) {
        while (isspace(inner.peek())) {
            inner.get();
        }

        if (!inner.peek()) {
            break;
        }

        inner.new_loc();
        last_loc = inner.loc();

        switch (*inner.peek()) {
        case '0' ... '9': {
            std::string value;
            do {
                value += *inner.get();
            } while (isdecimaldigit(inner.peek()));
            return Token::IntegerConstant(inner.loc(), static_cast<uintmax_t>(std::atoll(value.c_str())));
        }
        case '_':
        case 'a' ... 'z':
        case 'A' ... 'Z': {
            // TODO: Universal character names
            std::string identifier;
            do {
                identifier += *inner.get();
            } while (isdecimaldigit(inner.peek()) || isidentifiernondigit(inner.peek()));
            return Token::Identifier(inner.loc(), identifier);
        }
        case '[':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::LBracket);
        case ']':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::RBracket);
        case '(':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::LParen);
        case ')':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::RParen);
        case '{':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::LBrace);
        case '}':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::RBrace);
        case '.':
            inner.get();
            if (inner.consume_if('.')) {
                ASSERT(inner.consume_if('.'));
                return Token::Punctuator(inner.loc(), PunctuatorKind::DotDotDot);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Dot);
        case '&':
            inner.get();
            if (inner.consume_if('&')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::AndAnd);
            }
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::AndEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::And);
        case '|':
            inner.get();
            if (inner.consume_if('|')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::OrOr);
            }
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::OrEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Or);
        case '^':
            inner.get();
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::CaretEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Caret);
        case '~':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::Tilde);
        case '!':
            inner.get();
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::NotEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Not);
        case '+':
            inner.get();
            if (inner.consume_if('+')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::PlusPlus);
            }
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::PlusEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Plus);
        case '-':
            inner.get();
            if (inner.consume_if('-')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::MinusMinus);
            }
            if (inner.consume_if('>')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::Arrow);
            }
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::MinusEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Minus);
        case '*':
            inner.get();
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::StarEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Star);
        case '/':
            inner.get();
            if (inner.consume_if('/')) {
                ASSERT(!"Handle comment");
            }
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::SlashEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Slash);
        case '%':
            inner.get();
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::ModuloEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Modulo);
        case '<':
            inner.get();
            if (inner.consume_if('<')) {
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::LLAngleEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::LLAngle);
            }
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::LAngleEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::LAngle);
        case '>':
            inner.get();
            if (inner.consume_if('>')) {
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::RRAngleEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::RRAngle);
            }
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::RAngleEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::RAngle);
        case '?':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::Query);
        case ':':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::Colon);
        case ';':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::Semi);
        case '=':
            inner.get();
            if (inner.consume_if('=')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::EqEq);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Eq);
        case ',':
            inner.get();
            return Token::Punctuator(inner.loc(), PunctuatorKind::Comma);
        case '#':
            inner.get();
            if (inner.consume_if('#')) {
                return Token::Punctuator(inner.loc(), PunctuatorKind::HashHash);
            }
            return Token::Punctuator(inner.loc(), PunctuatorKind::Hash);
        }

        std::terminate();  // invalid character
    }

    return Token{
        TokenKind::EndOfFile,
        inner.loc(),
    };
}
