#include <parsing/parser.hh>
#include <diagnostics.hh>
#include <type/types.hh>
#include <charconv>

namespace Parsing
{
    Parser::Parser(const std::vector<Lexing::Token>& tokens, const std::string& text)
        :_text(text), _tokens(tokens), _position(0), _currentScope(std::make_shared<Environment>(nullptr)), _currentReturnType(nullptr)
    {
    }

    Lexing::Token Parser::Current() const
    {
        return _tokens[_position];
    }

    Lexing::Token Parser::Consume()
    {
        return _tokens[_position++];
    }

    Lexing::Token Parser::Peek(const int offset) const
    {
        return _tokens[_position + offset];
    }

    int Parser::GetBinOpPrecedence(Lexing::TokenType type)
    {
        switch(type)
        {
            case Lexing::TokenType::LeftSquareBracket:
                return 55;
            case Lexing::TokenType::Star:
            case Lexing::TokenType::Slash:
                return 40;
            
            case Lexing::TokenType::Plus:
            case Lexing::TokenType::Minus:
                return 35;

            case Lexing::TokenType::LeftAngleBracket:
            case Lexing::TokenType::RightAngleBracket:
                return 30;
                
            case Lexing::TokenType::DoubleEquals:
            case Lexing::TokenType::BangEquals:
                return 25;            
        
            case Lexing::TokenType::Equals:
                return 10;
            
            default:
                return 0;
        }
    }

    int Parser::GetUnOpPrecedence(Lexing::TokenType type)
    {
        switch(type)
        {
            case Lexing::TokenType::Minus:
            case Lexing::TokenType::Hash:
            case Lexing::TokenType::Asperand:
                return 50;
            default:
                return 0;
        }
    }

    void Parser::ExpectToken(Lexing::TokenType tokenType)
    {
        if(Current().GetType() != tokenType)
        {
            Lexing::Token temp(tokenType, "", 0, 0, 0, 0);

            ParserError("Expected '" + temp.TypeAsString() + "', found " + Current().GetText());
        }
    }

    void Parser::ParserError(std::string message)
    {
        unsigned int start = Current().GetStart();
        while(_text[start] != '\n')
            start--;
        unsigned int end = Current().GetEnd();
        while(_text[end] != '\n')
            end++;
        Diagnostics::CompilerError(Current().GetLine(), Current().GetCol(),
        message, &_text[Current().GetStart()], &_text[Current().GetEnd()],
                &_text[start], &_text[end]);
    }


    std::vector<std::unique_ptr<ASTNode>> Parser::Parse()
    {
        std::vector<std::unique_ptr<ASTNode>> result;
        while(_position < _tokens.size())
        {
            int savePos = _position;
            std::unique_ptr<ASTNode> expr = ParseExpression();
            ExpectToken(Lexing::TokenType::Semicolon);
            Consume();

            if(expr->GetNodeType() == ASTNodeType::Function || expr->GetNodeType() == ASTNodeType::ImportStatement)
                result.push_back(std::move(expr));
            else
            {
                _position = savePos;
                ParserError("Expected top-level expression");
            }
        }
        return result;
    }
    
    std::unique_ptr<ASTNode> Parser::ParseExpression(int precedence)
    {
        std::unique_ptr<ASTNode> lhs;
        int unOpPrecedence = GetUnOpPrecedence(Current().GetType());
        if(unOpPrecedence && unOpPrecedence >= precedence)
        {
            Lexing::Token operatorToken = Consume();
            lhs = std::make_unique<UnaryExpression>(ParseExpression(unOpPrecedence), operatorToken);
        }
        else
            lhs = ParsePrimary();

        while(true)
        {
            int binOpPrecedence = GetBinOpPrecedence(Current().GetType());
            if(binOpPrecedence < precedence)
                break;

            Lexing::Token operatorToken = Consume();
            std::unique_ptr<ASTNode> rhs = ParseExpression(binOpPrecedence);
            lhs = std::make_unique<BinaryExpression>(std::move(lhs), operatorToken, std::move(rhs));
            if(operatorToken.GetType() == Lexing::TokenType::LeftSquareBracket)
            {
                ExpectToken(Lexing::TokenType::RightSquareBracket);
                Consume();
            }
        }

        if(Current().GetType() == Lexing::TokenType::LeftParen)
            lhs = ParseCallExpression(std::move(lhs));

        return lhs;
    }

    std::unique_ptr<ASTNode> Parser::ParsePrimary()
    {
        switch(Current().GetType())
        {
            case Lexing::TokenType::Let:
                return ParseVariableDeclaration();
            case Lexing::TokenType::Return:
                return ParseReturnStatement();
            case Lexing::TokenType::Integer:
                return ParseIntegerLiteral();
            case Lexing::TokenType::String:
                return ParseStringLiteral();
            case Lexing::TokenType::Identifier:
                return ParseVariable();
            case Lexing::TokenType::LeftBracket:
                return ParseCompoundExpression();
            case Lexing::TokenType::LeftParen:
                return ParseParenthesizedExpression();
            case Lexing::TokenType::If:
                return ParseIfStatement();
            case Lexing::TokenType::While:
                return ParseWhileStatement();
            case Lexing::TokenType::Import:
                return ParseImportStatement();
            default:
                ParserError("Expected primary expression, found '" + Current().GetText() + "'");
        }
    }

    std::shared_ptr<Type> Parser::ParseType()
    {
        ExpectToken(Lexing::TokenType::Type);
        std::shared_ptr<Type> type = types.at(Consume().GetText());
        while(Current().GetType() == Lexing::TokenType::Star || Current().GetType() == Lexing::TokenType::LeftSquareBracket)
        {
            if(Current().GetType() == Lexing::TokenType::Star)
            {
                Consume();
                type = std::make_shared<PointerType>(type);
            }
            else
            {
                Consume();
                int length = std::stoi(Consume().GetText());
                type = std::make_shared<ArrayType>(length, type);

                ExpectToken(Lexing::TokenType::RightSquareBracket);
                Consume();
            }
        }
        return type;
    }

    std::unique_ptr<ASTNode> Parser::ParseVariableDeclaration()
    {
        Consume();

        std::shared_ptr<Type> type = ParseType();

        ExpectToken(Lexing::TokenType::Identifier);
        std::string name = Consume().GetText();
        
        _currentScope->GetVarSymbols().push_back(std::make_shared<VarSymbol>(name, type));

        std::vector<std::pair<std::shared_ptr<Type>, std::string>> args;
        std::shared_ptr<Environment> scope = nullptr;
        if(Current().GetType() == Lexing::TokenType::LeftParen)
        {
            Consume();
            scope = std::make_shared<Environment>(_currentScope);
            _currentScope = scope;
            while(Current().GetType() != Lexing::TokenType::RightParen)
            {
                std::shared_ptr<Type> type = ParseType();

                ExpectToken(Lexing::TokenType::Identifier);
                std::string argName = Consume().GetText();
                
                args.push_back(std::make_pair(type, argName));
                _currentScope->GetVarSymbols().push_back(std::make_shared<VarSymbol>(argName, type));
                if(Current().GetType() == Lexing::TokenType::RightParen)
                    break;

                ExpectToken(Lexing::TokenType::Comma);
                Consume();
            }
            Consume();
            _currentReturnType = type;
        }

        if(Current().GetType() != Lexing::TokenType::Equals)
        {
            if(scope)
                _currentScope = _currentScope->GetOuter();

            return std::make_unique<VariableDeclaration>(name, nullptr, scope, type, args);
        }

        Consume();
        
        std::unique_ptr<ASTNode> initVal = ParseExpression();

        if(scope)
            _currentScope = _currentScope->GetOuter();

        return std::make_unique<VariableDeclaration>(name, std::move(initVal), scope, type, args);
    }

    std::unique_ptr<ASTNode> Parser::ParseVariable()
    {
        std::string name = Consume().GetText();

        std::shared_ptr<VarSymbol> symbol = _currentScope->FindVarSymbol(name);

        return std::make_unique<Variable>(name, symbol->GetType());
    }

    std::unique_ptr<ASTNode> Parser::ParseImportStatement()
    {
        Consume();

        std::shared_ptr<Type> type = ParseType();

        std::string name = Consume().GetText();

        _currentScope->GetVarSymbols().push_back(std::make_shared<VarSymbol>(name, type));

        std::vector<std::pair<std::shared_ptr<Type>, std::string>> args;
        ExpectToken(Lexing::TokenType::LeftParen);
        Consume();
        while(Current().GetType() != Lexing::TokenType::RightParen)
        {
            std::shared_ptr<Type> type = ParseType();

            ExpectToken(Lexing::TokenType::Identifier);
            std::string argName = Consume().GetText();
            
            args.push_back(std::make_pair(type, argName));
            if(Current().GetType() == Lexing::TokenType::RightParen)
                break;

            ExpectToken(Lexing::TokenType::Comma);
            Consume();
        }
        Consume();

        return std::make_unique<ImportStatement>(name, type, std::move(args));
    }

    std::unique_ptr<ASTNode> Parser::ParseCallExpression(std::unique_ptr<ASTNode> callee)
    {
        Consume();
        std::vector<std::unique_ptr<ASTNode>> args;
        while(Current().GetType() != Lexing::TokenType::RightParen)
        {
            args.push_back(ParseExpression());
            if(Current().GetType() == Lexing::TokenType::RightParen)
                break;

            ExpectToken(Lexing::TokenType::Comma);
            Consume();
        }
        Consume();

        return std::make_unique<CallExpr>(std::move(callee), std::move(args));
    }

    std::unique_ptr<ASTNode> Parser::ParseIntegerLiteral()
    {
        long long value = std::stoll(Consume().GetText());

        return std::make_unique<IntegerLiteral>(value);
    }

    std::unique_ptr<ASTNode> Parser::ParseStringLiteral()
    {
        std::string value = Consume().GetText();

        return std::make_unique<StringLiteral>(value);
    }

    std::unique_ptr<ASTNode> Parser::ParseReturnStatement()
    {
        Consume();

        if(Current().GetType() == Lexing::TokenType::Semicolon)
            return std::make_unique<ReturnStatement>(nullptr, _currentReturnType);

        return std::make_unique<ReturnStatement>(ParseExpression(), _currentReturnType);
    }

    std::unique_ptr<ASTNode> Parser::ParseCompoundExpression()
    {
        Consume();

        std::shared_ptr<Environment> scope = std::make_shared<Environment>(_currentScope);
        _currentScope = scope;

        std::vector<std::unique_ptr<ASTNode>> exprs;

        while(Current().GetType() != Lexing::TokenType::RightBracket)
        {
            exprs.push_back(ParseExpression());
            ExpectToken(Lexing::TokenType::Semicolon);
            Consume();
        }
        Consume();

        _tokens.insert(_tokens.begin() + _position, Lexing::Token(Lexing::TokenType::Semicolon, "", 0, 0, 0, 0));

        _currentScope = scope->GetOuter();

        return std::make_unique<CompoundStatement>(exprs, scope);
    }

    std::unique_ptr<ASTNode> Parser::ParseParenthesizedExpression()
    {
        Consume();

        std::unique_ptr<ASTNode> expr = ParseExpression();

        ExpectToken(Lexing::TokenType::RightParen);
        Consume();

        return expr;
    }

    std::unique_ptr<ASTNode> Parser::ParseIfStatement()
    {
        Consume();

        ExpectToken(Lexing::TokenType::LeftParen);
        Consume();

        std::unique_ptr<ASTNode> cond = ParseExpression();

        ExpectToken(Lexing::TokenType::RightParen);
        Consume();

        std::unique_ptr<ASTNode> body = ParseExpression();

        if(Peek(1).GetType() == Lexing::TokenType::Else)
        {
            ExpectToken(Lexing::TokenType::Semicolon);
            Consume();
            Consume();
            return std::make_unique<IfStatement>(std::move(cond), std::move(body), ParseExpression());
        }

        return std::make_unique<IfStatement>(std::move(cond), std::move(body), nullptr);
    }

    std::unique_ptr<ASTNode> Parser::ParseWhileStatement()
    {
        Consume();

        ExpectToken(Lexing::TokenType::LeftParen);
        Consume();

        std::unique_ptr<ASTNode> cond = ParseExpression();

        ExpectToken(Lexing::TokenType::RightParen);
        Consume();

        std::unique_ptr<ASTNode> body = ParseExpression();

        return std::make_unique<WhileStatement>(std::move(cond), std::move(body));
    }
}