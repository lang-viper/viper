#ifndef QRK_AST_EXPR_HH
#define QRK_AST_EXPR_HH
#include <typing/types.hh>
#include <globals.hh>
#include <llvm/IR/Value.h>
#include <ostream>
#include <memory>
#include <string>

enum class expr_type
{
    BIN_OP, UN_OP,
    INT_LITERAL,

    RETURN,
    IF,
    FOR, WHILE, BREAK,
    LABEL, GOTO,

    VARIABLE, VARIABLE_DECL,

    CALL,

    COMPOUND,
};

class ast_expr
{
public:
    virtual ~ast_expr() {}

    virtual void print(std::ostream&) const = 0;
    virtual expr_type get_type() const = 0;
    virtual llvm::Value* codegen(std::shared_ptr<scope>) const = 0;

    friend std::ostream& operator<<(std::ostream&, ast_expr&);

    type_info type = types.at("void");
};

#endif