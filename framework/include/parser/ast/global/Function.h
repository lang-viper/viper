// Copyright 2024 solar-mist

#ifndef VIPER_FRAMEWORK_PARSER_AST_GLOBAL_FUNCTION_H
#define VIPER_FRAMEWORK_PARSER_AST_GLOBAL_FUNCTION_H 1

#include "parser/ast/ASTNode.h"

#include <memory>
#include <string>
#include <vector>

namespace parser
{
    class Function : public ASTNode
    {
    public:
        Function(std::string name, std::vector<ASTNodePtr> body, ScopePtr scope);

        virtual vipir::Value* codegen(vipir::IRBuilder& builder, vipir::Module& module, diagnostic::Diagnostics& diag) override;

    private:
        std::string mName;
        std::vector<ASTNodePtr> mBody;

        ScopePtr mOwnScope;
    };
    using FunctionPtr = std::unique_ptr<Function>;
}

#endif // VIPER_FRAMEWORK_PARSER_AST_GLOBAL_FUNCTION_H