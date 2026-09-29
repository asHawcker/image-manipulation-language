#include "parser.hpp"
#include <stdexcept>

void Parser::advance()
{
    curr_token = lexer.next_token();
}

bool Parser::match(TokenType type)
{
    if (type == curr_token.type)
        return true;
    return false;
}

Token Parser::consume(TokenType type, const std::string &err_msg)
{
    if (match(type))
    {
        Token tok = curr_token;
        advance();
        return tok;
    }
    else
    {
        throw std::runtime_error(err_msg);
    }
}

std::unique_ptr<Stmt> Parser::parse_stmt()
{
    if (match(TokenType::KW_int))
        return parse_var_dec_stmt();
    else
        return parse_expr_stmt();
}

std::unique_ptr<Stmt> Parser::parse_var_dec_stmt()
{
    advance();
    Token identifier = consume(TokenType::Identifier, "expected Identifier ");
    Token op = consume(TokenType::OP_equal, "expected '=' equals operator ");
    std::unique_ptr<Expr> val = std::move(parse_expr());
    consume(TokenType::Semicolon, "expected ';' semicolon");
    return std::make_unique<VarDecStmt>(identifier.lexeme, TokenType::KW_int, val);
}

std::unique_ptr<Expr> Parser::parse_primary()
{
    if (match(TokenType::Lit_int))
    {
        Token val = consume(TokenType::Lit_int, "expected an integer literal ");
        return std::make_unique<IntExpr>(stoi(val.lexeme));
    }

    if (match(TokenType::Identifier))
    {
        Token identifier = consume(TokenType::Identifier, "Expected identifer ");
        if (match(TokenType::Paren_left))
        {
            consume(TokenType::Paren_left, "");
            std::vector<std::unique_ptr<Expr>> args;
            while (!match(TokenType::Paren_right))
            {
                std::unique_ptr<Expr> arg = std::move(parse_expr());
                args.push_back(arg);
                if (match(TokenType::Comma))
                    consume(TokenType::Comma, "");
            }
            consume(TokenType::Paren_right, "");
            return std::make_unique<CallExpr>(identifier.lexeme, args);
        }
        return std::make_unique<VarExpr>(identifier.lexeme);
    }

    throw std::runtime_error("Unexpected token in expression ");
}

std::unique_ptr<Expr> Parser::parse_expr()
{
    std::unique_ptr<Expr> ex = std::move(parse_primary());
    if (match(TokenType::OP_plus) ||
        match(TokenType::OP_minus) ||
        match(TokenType::OP_star) ||
        match(TokenType::OP_slash) ||
        match(TokenType::OP_mod))
    {
        Token op = consume(curr_token.type, "");
        std::unique_ptr<Expr> ex2 = std::move(parse_primary());
        return std::make_unique<BinExpr>(op.type, std::move(ex), std::move(ex2));
    }
    return ex;
}

std::unique_ptr<Stmt> Parser::parse_expr_stmt()
{
    std::unique_ptr<Expr> val = std::move(parse_expr());
    consume(TokenType::Semicolon, "expected ';' semicolon");
    return std::make_unique<ExprStmt>(std::move(val));
}

std::vector<std::unique_ptr<Stmt>> Parser::parse_program()
{
    std::vector<std::unique_ptr<Stmt>> program;

    while (curr_token.type != TokenType::Eof)
    {
        std::unique_ptr<Stmt> statement = std::move(parse_stmt());
        program.push_back(statement);
    }
    return program;
}