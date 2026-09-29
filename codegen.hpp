#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <map>
#include <string>
#include <memory>

struct CodeGenContext
{
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;
    std::map<std::string, llvm::Value *> named_values;

    CodeGenContext() : builder(context)
    {
        module = std::make_unique<llvm::Module>("iml_compiler", context);
    }
};

#endif