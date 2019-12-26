#include "fmt/core.h"
#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"

namespace fmt {
    template<>
    struct formatter<C0::ErrorCode> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const C0::ErrorCode &p, FormatContext &ctx) {
            std::string name;
            switch (p) {
                case C0::ErrNoError:
                    name = "No error.";
                    break;
                case C0::ErrStreamError:
                    name = "Stream error.";
                    break;
                case C0::ErrEOF:
                    name = "EOF";
                    break;
                case C0::ErrInvalidInput:
                    name = "The input is invalid.";
                    break;
                case C0::ErrInvalidIdentifier:
                    name = "Identifier is invalid";
                    break;
                case C0::ErrIntegerOverflow:
                    name = "The integer is too big(int64_t).";
                    break;
                    break;
                case C0::ErrNoMain:
                    name = "The program should have 'main' function.";
                    break;
                case C0::ErrIncompleteFunctionDefinition:
                    name = "Function definition is incomplete.";
                    break;
                case C0::ErrInvalidScan:
                    name = "The scan statement is invalid.";
                    break;
                case C0::ErrInvalidPrint:
                    name = "The print statement is invalid.";
                    break;
                case C0::ErrNeedIdentifier:
                    name = "Need an identifier here.";
                    break;
                case C0::ErrInvalidConditionStatement:
                    name = "The condition statement is invalid.";
                    break;
                case C0::ErrInvalidFunctionCall:
                    name = "The function call is invalid.";
                    break;
                case C0::ErrInvalidStatement:
                    name = "The statement is invalid.";
                    break;
                case C0::ErrJumpStatement:
                    name = "The jump statement is invalid.";
                    break;
                case C0::ErrInvalidReturn:
                    name = "The return statement is invalid.";
                    break;


                case C0::ErrConstantNeedValue:
                    name = "The constant need a value to initialize.";
                    break;
                case C0::ErrNoSemicolon:
                    name = "Zai? Wei shen me bu xie fen hao.";
                    break;
                case C0::ErrInvalidVariableDeclaration:
                    name = "The declaration is invalid.";
                    break;
                case C0::ErrIncompleteExpression:
                    name = "The expression is incomplete.";
                    break;
                case C0::ErrNotDeclared:
                    name = "The variable or constant must be declared before being used.";
                    break;
                case C0::ErrAssignToConstant:
                    name = "Trying to assign value to a constant.";
                    break;
                case C0::ErrDuplicateDeclaration:
                    name = "The variable or constant has been declared.";
                    break;
                case C0::ErrNotInitialized:
                    name = "The variable has not been initialized.";
                    break;
                case C0::ErrInvalidAssignment:
                    name = "The assignment statement is invalid.";
                    break;
            }
            return format_to(ctx.out(), name);
        }
    };

    template<>
    struct formatter<C0::CompilationError> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const C0::CompilationError &p, FormatContext &ctx) {
            return format_to(ctx.out(), "Line: {} Column: {} Error: {}", p.GetPos().first, p.GetPos().second,
                             p.GetCode());
        }
    };
}

namespace fmt {
    template<>
    struct formatter<C0::Token> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const C0::Token &p, FormatContext &ctx) {
            return format_to(ctx.out(),
                             "Line: {} Column: {} Type: {} Value: {}",
                             p.GetStartPos().first, p.GetStartPos().second, p.GetType(), p.GetValueString());
        }
    };

    template<>
    struct formatter<C0::TokenType> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const C0::TokenType &p, FormatContext &ctx) {
            std::string name;
            switch (p) {
                case C0::NULL_TOKEN:
                    name = "NullToken";
                    break;
                case C0::DECIMAL:
                    name = "Decimal";
                    break;
                case C0::HEXADECIMAL:
                    name = "HexaDecimal";
                    break;
                case C0::IDENTIFIER:
                    name = "Identifier";
                    break;
                case C0::VOID:
                    name = "Void";
                    break;
                case C0::INT:
                    name = "Int";
                    break;
                case C0::DOUBLE:
                    name = "Double";
                    break;
                case C0::CONST:
                    name = "Const";
                    break;
                case C0::CHAR:
                    name = "Char";
                    break;
                case C0::STRUCT:
                    name = "Struct";
                    break;
                case C0::IF:
                    name = "If";
                    break;
                case C0::ELSE:
                    name = "Else";
                    break;
                case C0::SWITCH:
                    name = "Switch";
                    break;
                case C0::CASE:
                    name = "Case";
                    break;
                case C0::DEFAULT:
                    name = "Default";
                    break;
                case C0::WHILE:
                    name = "While";
                    break;
                case C0::DO:
                    name = "Do";
                    break;
                case C0::FOR:
                    name = "For";
                    break;
                case C0::RETURN:
                    name = "Return";
                    break;
                case C0::BREAK:
                    name = "Break";
                    break;
                case C0::CONTINUE:
                    name = "Continue";
                    break;
                case C0::PRINT:
                    name = "Print";
                    break;
                case C0::SCAN:
                    name = "Scan";
                    break;
                case C0::PLUS_SIGN:
                    name = "PlusSign";
                    break;
                case C0::MINUS_SIGN:
                    name = "MinusSign";
                    break;
                case C0::MULTIPLICATION_SIGN:
                    name = "MultiplicationSign";
                    break;
                case C0::DIVISION_SIGN:
                    name = "DivisionSign";
                    break;
                case C0::EQUAL_SIGN:
                    name = "EqualSign";
                    break;
                case C0::SEMICOLON:
                    name = "Semicolon";
                    break;
                case C0::LEFT_BRACKET:
                    name = "LeftBracket";
                    break;
                case C0::RIGHT_BRACKET:
                    name = "RightBracket";
                    break;
                case C0::LESS_SIGN:
                    name = "LessSign";
                    break;
                case C0::LESS_EQUAL_SIGN:
                    name = "LessEqualSign";
                    break;
                case C0::MORE_SIGN:
                    name = "MoreSign";
                    break;
                case C0::MORE_EQUAL_SIGN:
                    name = "MoreEqualSign";
                    break;
                case C0::EQUAL_EQUAL_SIGN:
                    name = "EqualEqualSign";
                    break;
                case C0::NOT_EQUAL_SIGN:
                    name = "NotEqualSign";
                    break;
                case C0::LEFT_BRACE:
                    name = "LeftBrace";
                    break;
                case C0::RIGHT_BRACE:
                    name = "RightBrace";
                    break;
                case C0::COMMA:
                    name = "Comma";
                    break;
                case C0::COMMENT:
                    name = "Comment";
                    break;
            }
            return format_to(ctx.out(), name);
        }
    };
}

namespace fmt {
    template<>
    struct formatter<C0::Operation> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const C0::Operation &p, FormatContext &ctx) {
            std::string name;
            switch (p) {
                case C0::snew:
                    name = "snew";
                    break;
                case C0::ipush:
                    name = "ipush";
                    break;
                case C0::bipush:
                    name = "bipush";
                    break;
                case C0::loada:
                    name = "loada";
                    break;
                case C0::iload:
                    name = "iload";
                    break;
                case C0::istore:
                    name = "istore";
                    break;
                case C0::iprint:
                    name = "iprint";
                    break;
                case C0::printl:
                    name = "printl";
                    break;
                case C0::cprint:
                    name = "cprint";
                    break;
                case C0::iadd:
                    name = "iadd";
                    break;
                case C0::isub:
                    name = "isub";
                    break;
                case C0::imul:
                    name = "imul";
                    break;
                case C0::idiv:
                    name = "idiv";
                    break;
                case C0::iret:
                    name = "iret";
                    break;
                case C0::ret:
                    name = "ret";
                    break;
                case C0::ineg:
                    name = "ineg";
                    break;
                case C0::loadc:
                    name = "loadc";
                    break;
                case C0::icmp:
                    name = "icmp";
                    break;
                case C0::jge:
                    name = "jge";
                    break;
                case C0::je:
                    name = "je";
                    break;
                case C0::jne:
                    name = "jne";
                    break;
                case C0::jl:
                    name = "jl";
                    break;
                case C0::jle:
                    name = "jle";
                    break;
                case C0::jg:
                    name = "jg";
                    break;
                case C0::jmp:
                    name = "jmp";
                    break;
                case C0::call:
                    name = "call";
                    break;
                case C0::iscan:
                    name = "iscan";
                    break;


            }
            return format_to(ctx.out(), name);
        }
    };

    template<>
    struct formatter<C0::Instruction> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const C0::Instruction &p, FormatContext &ctx) {
            //unsure 指令输出
            std::string name;
            switch (p.GetOperation()) {
                case C0::iload:
                case C0::istore:
                case C0::iprint:
                case C0::cprint:
                case C0::printl:
                case C0::iadd:
                case C0::isub:
                case C0::imul:
                case C0::idiv:
                case C0::iret:
                case C0::ret:
                case C0::iscan:
                case C0::ineg:
                case C0::icmp:
                    return format_to(ctx.out(), "{}", p.GetOperation());
                case C0::snew:
                case C0::loadc:
                case C0::ipush:
                case C0::bipush:
                case C0::jge:
                case C0::jg:
                case C0::je:
                case C0::jne:
                case C0::jl:
                case C0::jle:
                case C0::jmp:
                case C0::call:
                    return format_to(ctx.out(), "{} {}", p.GetOperation(), p.GetX());
                case C0::loada:
                    return format_to(ctx.out(), "{} {},{}", p.GetOperation(), p.GetX(),p.GetY());
            }
            //意义？ undone
            return format_to(ctx.out(), "snew");
        }
    };
}