#include "ast.hpp"
#include "codegen.hpp"

void ExprStmt::codegen(CodeGenContext &context)
{
    expression->codegen(context);
}

llvm::Value *IntExpr::codegen(CodeGenContext &context)
{
    return llvm::ConstantInt::get(context.context, llvm::APInt(32, value, true));
}
llvm::Value *FloatExpr::codegen(CodeGenContext &context)
{
    return llvm::ConstantFP::get(context.context, llvm::APFloat(value));
}
llvm::Value *BoolExpr::codegen(CodeGenContext &context)
{
    return llvm::ConstantInt::getBool(context.context, value);
}

llvm::Value *BinExpr::codegen(CodeGenContext &context)
{
    llvm::Value *L = left->codegen(context);
    llvm::Value *R = right->codegen(context);

    if (L == nullptr || R == nullptr)
    {
        return nullptr;
    }

    if (this->type == Type::FLOAT)
    {
        if (left->type == Type::INT)
        {
            context.builder.CreateSIToFP(L, llvm::Type::getFloatTy(context.context), "castL");
        }
        if (right->type == Type::INT)
        {
            context.builder.CreateSIToFP(R, llvm::Type::getFloatTy(context.context), "castR");
        }
        switch (op)
        {
        case TokenType::OP_plus:
            return context.builder.CreateFAdd(L, R, "faddtmp");
        case TokenType::OP_minus:
            return context.builder.CreateFSub(L, R, "fsubtmp");
        case TokenType::OP_star:
            return context.builder.CreateFMul(L, R, "fmultmp");
        case TokenType::OP_slash:
            return context.builder.CreateFDiv(L, R, "fdivtmp");
        default:
            return nullptr;
        }
    }
    else if (this->type == Type::INT)
    {
        switch (op)
        {
        case TokenType::OP_plus:
            return context.builder.CreateAdd(L, R, "addtmp");
        case TokenType::OP_minus:
            return context.builder.CreateSub(L, R, "subtmp");
        case TokenType::OP_star:
            return context.builder.CreateMul(L, R, "multmp");
        case TokenType::OP_slash:
            return context.builder.CreateSDiv(L, R, "divtmp");
        default:
            return nullptr;
        }
    }
    return nullptr;
}

llvm::Value *VarExpr::codegen(CodeGenContext &context)
{
    if (context.named_values.find(value) != context.named_values.end())
    {
        llvm::Value *ptr = context.named_values[value];
        llvm::AllocaInst *alloca = llvm::cast<llvm::AllocaInst>(ptr);
        return context.builder.CreateLoad(alloca->getAllocatedType(), ptr, value.c_str());
    }
    return nullptr;
}

void VarDecStmt::codegen(CodeGenContext &context)
{
    llvm::Value *init_value = value->codegen(context);
    if (!init_value)
        return;
    if (type == TokenType::KW_float && init_value->getType()->isIntegerTy())
        init_value = context.builder.CreateSIToFP(init_value, llvm::Type::getFloatTy(context.context), "casttmp");

    llvm::AllocaInst *alloca;
    if (type == TokenType::KW_int)
        alloca = context.builder.CreateAlloca(llvm::Type::getInt32Ty(context.context), nullptr, name);
    else if (type == TokenType::KW_float)
        alloca = context.builder.CreateAlloca(llvm::Type::getFloatTy(context.context), nullptr, name);
    else if (type == TokenType::KW_bool)
        alloca = context.builder.CreateAlloca(llvm::Type::getInt1Ty(context.context), nullptr, name);
    context.builder.CreateStore(init_value, alloca);
    context.named_values[name] = alloca;
}

llvm::Value *CallExpr::codegen(CodeGenContext &context)
{
    llvm::Function *callee = context.module->getFunction(name);
    if (!callee)
    {
        std::runtime_error("unknown function referenced ");
    }

    std::vector<llvm::Value *> argument_values;
    for (auto &arg : args)
    {
        llvm::Value *tmp = arg->codegen(context);
        if (!tmp)
            return nullptr;
        argument_values.push_back(tmp);
    }

    return context.builder.CreateCall(callee, argument_values, "calltmp");
}