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
    std::vector<std::map<std::string, llvm::Value *>> scopes;

    CodeGenContext() : builder(context)
    {
        module = std::make_unique<llvm::Module>("iml_compiler", context);
        scopes.push_back({});
    }

    void push_scope()
    {
        scopes.push_back({});
    }
    void pop_scope()
    {
        scopes.pop_back();
    }
    void set_var(std::string name, llvm::Value *value)
    {
        scopes.back()[name] = value;
    }

    llvm::Value *get_var(std::string name)
    {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++)
        {
            if (it->find(name) != it->end())
            {
                return it->at(name);
            }
        }
        return nullptr;
    }
};

#endif