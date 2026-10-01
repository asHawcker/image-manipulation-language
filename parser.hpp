#ifndef PARSER_H
#define PARSER_H

#include "ast.hpp"

class Parser
{
private:
    Lexer lexer;
    Token curr_token;

    int get_token_precedence(TokenType type);

public:
    Parser(Lexer lexer) : lexer(lexer)
    {
        advance();
    }
    void advance();
    bool match(TokenType type);
    Token consume(TokenType type, const std::string &err_msg);

    // parsing methods
    std::unique_ptr<Stmt> parse_stmt();
    std::unique_ptr<Stmt> parse_var_dec_stmt(TokenType tokentype);
    std::unique_ptr<Stmt> parse_expr_stmt();

    std::unique_ptr<Stmt> parse_block_stmt();
    std::unique_ptr<Stmt> parse_if_stmt();
    std::unique_ptr<Stmt> parse_while_stmt();

    std::unique_ptr<Expr> parse_expr(int min_precedence = 0);
    std::unique_ptr<Expr> parse_primary();

    std::vector<std::unique_ptr<Stmt>> parse_program();
};

#endif