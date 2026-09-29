#ifndef AST_H
#define AST_H

#include "lexer.hpp"
#include <memory>
#include <vector>
#include <llvm/IR/Value.h>

struct CodeGenContext;

class Expr
{
public:
    virtual ~Expr() = default;
    virtual llvm::Value *codegen(CodeGenContext &context) = 0;
};

class Stmt
{
public:
    virtual ~Stmt() = default;
    virtual void codegen(CodeGenContext &context) = 0;
};

class IntExpr : public Expr
{
public:
    int value;
    IntExpr(int value) : value(value) {}
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class VarExpr : public Expr
{
public:
    std::string value;
    VarExpr(std::string value) : value(std::move(value)) {}
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class BinExpr : public Expr
{
public:
    TokenType op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    BinExpr(TokenType op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right) : op(op), left(std::move(left)), right(std::move(right)) {}
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class CallExpr : public Expr
{
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(std::string name, std::vector<std::unique_ptr<Expr>> args) : name(std::move(name)), args(std::move(args)) {}
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class VarDecStmt : public Stmt
{
public:
    std::string name;
    TokenType type;
    std::unique_ptr<Expr> value;
    VarDecStmt(std::string name, TokenType type, std::unique_ptr<Expr> value) : name(std::move(name)), type(type), value(std::move(value)) {}
    virtual void codegen(CodeGenContext &context) override;
};

class ExprStmt : public Stmt
{
public:
    std::unique_ptr<Expr> expression;
    ExprStmt(std::unique_ptr<Expr> expression) : expression(std::move(expression)) {}
    virtual void codegen(CodeGenContext &context) override;
};

#endif