#ifndef LEXER_H
#define LEXER_H

#include <string>

enum class TokenType
{
    Eof,
    Identifier,

    Lit_string,
    Lit_int,
    Lit_float,

    OP_plus,
    OP_minus,
    OP_star,
    OP_slash,
    OP_mod,
    OP_equal,
    OP_dequal,
    OP_lt,
    OP_gt,
    OP_ltequal,
    OP_gtequal,

    OP_andand,
    OP_oror,
    OP_not,

    KW_int,

    KW_true,
    KW_false,

    Paren_left,
    Paren_right,
    Brace_left,
    Brace_right,

    Comma,
    Semicolon,
    Colon
};

struct Token
{
    TokenType type;
    std::string lexeme;
    std::size_t row;
    std::size_t col;
};

class Lexer
{
private:
    std::size_t row;
    std::size_t col;
    std::size_t cursor;
    std::string source;
    std::size_t length;

public:
    Lexer(std::string source_code);
    Token next_token();
};

#endif