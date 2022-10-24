#ifndef VIPER_PARSER_HH
#define VIPER_PARSER_HH
#include <parsing/ast/ast.hh>
#include <lexing/token.hh>
#include <vector>

namespace Parsing
{
    class Parser
    {
    public:
        Parser(const std::vector<Lexing::Token>& tokens, const std::string& text);

        std::vector<std::unique_ptr<ASTNode>> Parse();
    private:
        std::string _text;
        std::vector<Lexing::Token> _tokens;
        unsigned int _position;
        std::shared_ptr<Type> _currentReturnType;
        Environment* _currentScope;

        Lexing::Token Current() const;
        Lexing::Token Consume();
        Lexing::Token Peek(const int offset) const;

        int GetBinOpPrecedence(Lexing::TokenType type);
        int GetUnOpPrecedence(Lexing::TokenType type);

        void ExpectToken(Lexing::TokenType tokenType);
        [[noreturn]] void ParserError(std::string message);
        
        std::shared_ptr<Type> ParseType();

        std::unique_ptr<ASTNode> ParseExpression(int precedence = 1);
        std::unique_ptr<ASTNode> ParsePrimary();
        std::unique_ptr<ASTNode> ParseIdentifier();

        std::unique_ptr<ASTNode> ParseVariableDeclaration();
        std::unique_ptr<ASTNode> ParseVariable();

        std::unique_ptr<ASTNode> ParseImportStatement();

        std::unique_ptr<ASTNode> ParseCallExpression();

        std::unique_ptr<ASTNode> ParseIntegerLiteral();

        std::unique_ptr<ASTNode> ParseReturnStatement();

        std::unique_ptr<ASTNode> ParseParenthesizedExpression();
        std::unique_ptr<ASTNode> ParseCompoundExpression();
    };
}

#endif