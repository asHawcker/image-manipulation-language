#include "lexer.hpp"

Lexer::Lexer(std::string source_code)
{
    source = source_code;
    row = 1;
    col = 1;
    cursor = 0;
    length = source.length();
}

Token Lexer::next_token()
{
    Token next;
    next.row = row;
    next.col = col;

    while (1)
    {
        if (length <= cursor)
        {
            next.type = TokenType::Eof;
            next.lexeme = '\0';
            return next;
        }

        switch (source[cursor])
        {
        case ' ':
        case '\r':
        case '\t':
            col++;
            cursor++;
            continue;
        case '\n':
            row++;
            col = 1;
            cursor++;
            continue;
        }
        break;
    }

    next.row = row;
    next.col = col;

    switch (source[cursor])
    {
    case '\0':
        next.type = TokenType::Eof;
        next.lexeme = "";
        col++;
        cursor++;
        return next;
    case '+':
        next.type = TokenType::OP_plus;
        next.lexeme = source.substr(cursor, 1);
        col++;
        cursor++;
        return next;
    case '-':
        next.type = TokenType::OP_minus;
        next.lexeme = source.substr(cursor, 1);
        col++;
        cursor++;
        return next;
    case '*':
        next.type = TokenType::OP_star;
        next.lexeme = source.substr(cursor, 1);
        col++;
        cursor++;
        return next;
    case '/':
        next.type = TokenType::OP_slash;
        next.lexeme = source.substr(cursor, 1);
        col++;
        cursor++;
        return next;
    case '=':
        next.type = TokenType::OP_equal;
        next.lexeme = source.substr(cursor, 1);
        col++;
        cursor++;
        return next;
    case '(':
        next.type = TokenType::Paren_left;
        next.lexeme = source.substr(cursor, 1);
        col++;
        cursor++;
        return next;
    case ')':
        next.type = TokenType::Paren_right;
        next.lexeme = source.substr(cursor, 1);
        col++;
        cursor++;
        return next;
    case ';':
        next.type = TokenType::Semicolon;
        next.lexeme = source.substr(cursor, 1);
        col++;
        cursor++;
        return next;
    }

    if (isalpha(source[cursor]))
    {
        std::size_t l_word = 0;
        std::size_t start = cursor;
        while (cursor < length && isalnum(source[cursor]))
        {
            l_word++;
            cursor++;
        }

        col += l_word;

        next.lexeme = source.substr(start, l_word);

        if (next.lexeme == "int")
        {
            next.type = TokenType::KW_int;
        }
        else
        {
            next.type = TokenType::Identifier;
        }
        return next;
    }

    if (isdigit(source[cursor]))
    {
        std::size_t l_word = 0;
        std::size_t start = cursor;
        while (cursor < length && isdigit(source[cursor]))
        {
            l_word++;
            cursor++;
        }

        col += l_word;

        next.lexeme = source.substr(start, l_word);
        next.type = TokenType::Lit_int;

        return next;
    }

    return next;
}