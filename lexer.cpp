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
    case '{':
        next.type = TokenType::Brace_left;
        next.lexeme = "{";
        col++;
        cursor++;
        return next;
    case '}':
        next.type = TokenType::Brace_right;
        next.lexeme = "}";
        col++;
        cursor++;
        return next;
    case '<':
        if (cursor + 1 < length && source[cursor + 1] == '=')
        {
            next.type = TokenType::OP_ltequal;
            next.lexeme = "<=";
            col += 2;
            cursor += 2;
            return next;
        }
        next.type = TokenType::OP_lt;
        next.lexeme = "<";
        col++;
        cursor++;
        return next;
    case '>':
        if (cursor + 1 < length && source[cursor + 1] == '=')
        {
            next.type = TokenType::OP_gtequal;
            next.lexeme = ">=";
            col += 2;
            cursor += 2;
            return next;
        }
        next.type = TokenType::OP_gt;
        next.lexeme = ">";
        col++;
        cursor++;
        return next;
    case '=':
        // Handle '==' vs '='
        if (cursor + 1 < length && source[cursor + 1] == '=')
        {
            next.type = TokenType::OP_dequal;
            next.lexeme = "==";
            col += 2;
            cursor += 2;
            return next;
        }
        next.type = TokenType::OP_equal;
        next.lexeme = "=";
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
            next.type = TokenType::KW_int;
        else if (next.lexeme == "float")
            next.type = TokenType::KW_float;
        else if (next.lexeme == "bool")
            next.type = TokenType::KW_bool;
        else if (next.lexeme == "true")
            next.type = TokenType::KW_true;
        else if (next.lexeme == "false")
            next.type = TokenType::KW_false;
        else if (next.lexeme == "if")
            next.type = TokenType::KW_if;
        else if (next.lexeme == "else")
            next.type = TokenType::KW_else;
        else if (next.lexeme == "while")
            next.type = TokenType::KW_while;
        else if (next.lexeme == "extern")
            next.type = TokenType::KW_extern;
        else
            next.type = TokenType::Identifier;
        return next;
    }

    if (isdigit(source[cursor]))
    {
        std::size_t l_word = 0;
        std::size_t start = cursor;
        bool isFloat = false;
        while (cursor < length && (isdigit(source[cursor]) || source[cursor] == '.'))
        {
            if (source[cursor] == '.')
            {
                if (!isFloat)
                {
                    isFloat = true;
                    if (cursor + 1 < length && !isdigit(source[cursor + 1]))
                    {
                        next.type = TokenType::Error;
                        next.lexeme = "invalid float";
                        return next;
                    }
                }
                else
                {
                    next.type = TokenType::Error;
                    next.lexeme = "invalid float";
                    return next;
                }
            }
            l_word++;
            cursor++;
        }

        col += l_word;

        next.lexeme = source.substr(start, l_word);
        if (isFloat)
            next.type = TokenType::Lit_float;
        else
            next.type = TokenType::Lit_int;

        return next;
    }

    return next;
}

Token Lexer::peek_token()
{
    std::size_t prev_row = row;
    std::size_t prev_col = col;
    std::size_t prev_cursor = cursor;

    Token tok = next_token();

    row = prev_row;
    col = prev_col;
    cursor = prev_cursor;

    return tok;
}