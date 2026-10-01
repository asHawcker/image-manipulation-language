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
    if (!L || !R)
        return nullptr;

    bool is_float = (left->type == Type::FLOAT || right->type == Type::FLOAT);

    if (is_float)
    {
        if (left->type == Type::INT)
            L = context.builder.CreateSIToFP(L, llvm::Type::getFloatTy(context.context), "castL");
        if (right->type == Type::INT)
            R = context.builder.CreateSIToFP(R, llvm::Type::getFloatTy(context.context), "castR");

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
        case TokenType::OP_lt:
            return context.builder.CreateFCmpOLT(L, R, "cmplttmp");
        case TokenType::OP_gt:
            return context.builder.CreateFCmpOGT(L, R, "cmpgttmp");
        case TokenType::OP_ltequal:
            return context.builder.CreateFCmpOLE(L, R, "cmpltetmp");
        case TokenType::OP_gtequal:
            return context.builder.CreateFCmpOGE(L, R, "cmpgtetmp");
        case TokenType::OP_dequal:
            return context.builder.CreateFCmpOEQ(L, R, "cmpeqtmp");
        default:
            return nullptr;
        }
    }
    else
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
        case TokenType::OP_lt:
            return context.builder.CreateICmpSLT(L, R, "cmplttmp");
        case TokenType::OP_gt:
            return context.builder.CreateICmpSGT(L, R, "cmpgttmp");
        case TokenType::OP_ltequal:
            return context.builder.CreateICmpSLE(L, R, "cmpltetmp");
        case TokenType::OP_gtequal:
            return context.builder.CreateICmpSGE(L, R, "cmpgtetmp");
        case TokenType::OP_dequal:
            return context.builder.CreateICmpEQ(L, R, "cmpeqtmp");
        default:
            return nullptr;
        }
    }
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

void BlockStmt::codegen(CodeGenContext &context)
{
    for (auto &stmt : stmts)
    {
        stmt->codegen(context);
    }
}

void IfStmt::codegen(CodeGenContext &context)
{
    llvm::Value *condition = cond->codegen(context);
    if (!condition)
        return;

    llvm::Function *parent_func = context.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *true_block = llvm::BasicBlock::Create(context.context, "then", parent_func, nullptr);
    llvm::BasicBlock *false_block = llvm::BasicBlock::Create(context.context, "else");
    llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(context.context, "continue");

    context.builder.CreateCondBr(condition, true_block, false_block);

    // then block code
    context.builder.SetInsertPoint(true_block);
    branch_true->codegen(context);
    context.builder.CreateBr(merge_block);

    // else block code
    parent_func->insert(parent_func->end(), false_block);
    context.builder.SetInsertPoint(false_block);
    if (branch_false)
    {
        branch_false->codegen(context);
    }
    context.builder.CreateBr(merge_block);

    // merge
    parent_func->insert(parent_func->end(), merge_block);
    context.builder.SetInsertPoint(merge_block);
}

void WhileStmt::codegen(CodeGenContext &context)
{

    llvm::Function *parent_func = context.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *cond_block = llvm::BasicBlock::Create(context.context, "whilecond", parent_func);
    llvm::BasicBlock *body_block = llvm::BasicBlock::Create(context.context, "whilebody");
    llvm::BasicBlock *end = llvm::BasicBlock::Create(context.context, "whileend");

    context.builder.CreateBr(cond_block);
    context.builder.SetInsertPoint(cond_block);
    llvm::Value *condition = cond->codegen(context);
    if (!condition)
        return;
    context.builder.CreateCondBr(condition, body_block, end);

    parent_func->insert(parent_func->end(), body_block);
    context.builder.SetInsertPoint(body_block);
    body->codegen(context);
    context.builder.CreateBr(cond_block);

    parent_func->insert(parent_func->end(), end);
    context.builder.SetInsertPoint(end);
}