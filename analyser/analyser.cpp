#include "analyser.h"

#include <climits>
#include <string>
#include <vector>


namespace C0 {
    std::string area = "global";

    std::pair<Analyser::anaResult, std::optional<CompilationError>> Analyser::Analyse() {
        auto err = analyseProgram();
        if (err.has_value())
            return std::make_pair(result, err);
        else {
            result.cons = constv;
            result.func = funcv;
            result.ins = _instructions;
            return std::make_pair(result, std::optional<CompilationError>());
        }
    }

    // <C0-program> ::= {<variable-declaration>}{<function-definition>}
    // <function-definition> ::= <type-specifier><identifier><parameter-clause><compound-statement>
    std::optional<CompilationError> Analyser::analyseProgram() {
        while (true) {
            auto next = nextToken();
            if (!next.has_value()) {
                break;
            }

            if (next.value().GetType() == TokenType::CONST) {
                unreadToken();
                auto err = analyseConstantDeclaration();
                if (err.has_value())
                    return err;
                continue;
            } else if (next.value().GetType() == TokenType::INT) {
                next = nextToken();
                if (!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
                if (next.value().GetType() == TokenType::IDENTIFIER) {
                    next = nextToken();
                    if (!next.has_value())
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
                    if (next.value().GetType() == TokenType::EQUAL_SIGN ||
                        next.value().GetType() == TokenType::SEMICOLON || next.value().GetType() == TokenType::COMMA) {
                        unreadToken();
                        unreadToken();
                        unreadToken();
                        auto err = analyseVariableDeclaration();
                        if (err.has_value())
                            return err;
                        continue;
                    } else {
                        unreadToken();
                        unreadToken();
                        unreadToken();
                        break;
                    }
                } else {
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
                }
            } else if (next.value().GetType() == TokenType::VOID) {
                unreadToken();
                break;
            } else {
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidInput);
            }
        }
        while (true) {
            auto next = nextToken();
            if (!next.has_value()) {
                break;
            }
            if (next.value().GetType() == TokenType::VOID || next.value().GetType() == TokenType::INT) {
                next = nextToken();
                if (!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
                if (next.value().GetType() == TokenType::IDENTIFIER) {
                    next = nextToken();
                    if (!next.has_value())
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
                    if (next.value().GetType() == TokenType::LEFT_BRACKET) {
                        unreadToken();
                        unreadToken();
                        unreadToken();
                        auto err = analyseFunctionDefinition();
                        if (err.has_value())
                            return err;
                        continue;
                    } else {
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidInput);
                    }
                } else {
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
                }
            } else {
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidInput);
            }
        }
        if (!nymain) {
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoMain);
        }
        return {};
    }


    std::optional<CompilationError> Analyser::analyseConstantDeclaration() {
        auto next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::CONST) {
            unreadToken();
            return {};
        }
        next = nextToken();
        if (next.value().GetType() != TokenType::INT)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);

        //全局变量分配空间 UNSURE

        auto err = analyseConstDeclarator();
        if (err.has_value())
            return err;
        while (true) {
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::COMMA) {
                unreadToken();
                break;
            }
            err = analyseConstDeclarator();
            if (err.has_value())
                return err;
        }

        // ';'
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        return {};
    }

    //<init-declarator> ::= <identifier>[<initializer>]
    //<initializer> ::=  '='<expression>
    std::optional<CompilationError> Analyser::analyseConstDeclarator() {
        auto next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
        //作用域判断 unsure
        if (isLocal(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);

        //作用域判断 unsure
        if (area.compare("global") == 0)
            addConstant(next.value());
        else {
            auto &fun = funcv.at(getFunctionIndex(area));
            fun._consts[next.value().GetValueString()] = fun._nextTokenIndex++;
        }


        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);

        auto err = analyseExpression();
        if (err.has_value())
            return err;

        return {};
    }

    // <variable-declaration> ::=<type-specifier><init-declarator-list>';'
    //<init-declarator-list> ::=  <init-declarator>{','<init-declarator>}
    //<init-declarator> ::= <identifier>[<initializer>]
    //<initializer> ::=  '='<expression>
    std::optional<CompilationError> Analyser::analyseVariableDeclaration() {
        auto next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::INT) {
            unreadToken();
            return {};
        }

        //加指令 的多参数 是否需要修改指令的底层？ UNsure
//        auto type = next.value().GetType();

        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
        //作用域判断 unsure
        if (isLocal(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);

        auto var = next.value();
        next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
        // '='
        if (next.value().GetType() == TokenType::EQUAL_SIGN) {
            // '<表达式>'
            auto err = analyseExpression();
            if (err.has_value())
                return err;

            if (area.compare("global") == 0)
                addVariable(var);
            else {
                auto &fun = funcv.at(getFunctionIndex(area));
                fun._vars[var.GetValueString()] = fun._nextTokenIndex++;
            }

            //2.变量作用域层次差， 3.偏移地址 undone
            //全局变量与局部变量的指令？ undone
        } else {
            if (area.compare("global") == 0) {
                addUninitializedVariable(var);
                _instructions.emplace_back(snew, 1);
            } else {
                auto &fun = funcv.at(getFunctionIndex(area));
                fun._uninitialized_vars[var.GetValueString()] = fun._nextTokenIndex++;
                fun._instructions.emplace_back(snew, 1);
            }
            unreadToken();
        }

        next = nextToken();
        // ';'
        //重新考虑带 ， 的情况 unsure
        if (!next.has_value() || (next.value().GetType() != TokenType::SEMICOLON &&
                                  next.value().GetType() != TokenType::COMMA))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        if (next.value().GetType() == TokenType::COMMA) {
            unreadToken();
            while (true) {
                next = nextToken();
                if (!next.has_value())
                    break;
                if (next.value().GetType() != TokenType::COMMA) {
                    unreadToken();
                    break;
                }
                next = nextToken();
                if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
                //作用域判断 unsure
                if (isLocal(next.value().GetValueString()))
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
                auto var2 = next.value();
                next = nextToken();
                if (!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
                // '='
                if (next.value().GetType() == TokenType::EQUAL_SIGN) {
                    // '<表达式>'
                    auto err = analyseExpression();
                    if (err.has_value())
                        return err;
                    if (area.compare("global") == 0)
                        addVariable(var2);
                    else {
                        auto &fun = funcv.at(getFunctionIndex(area));
                        fun._vars[var2.GetValueString()] = fun._nextTokenIndex++;
                    }
                    //2.变量作用域层次差， 3.偏移地址 undone
                    //全局变量与局部变量的指令？ undone
                } else {
                    if (area.compare("global") == 0) {
                        addUninitializedVariable(var2);
                        _instructions.emplace_back(snew, 1);
                    } else {
                        auto &fun = funcv.at(getFunctionIndex(area));
                        fun._uninitialized_vars[var2.GetValueString()] = fun._nextTokenIndex++;
                        fun._instructions.emplace_back(snew, 1);
                    }
                    unreadToken();
                }
            }
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
        }
        return {};
    }

    //<function-definition>::=<type-specifier><identifier><parameter-clause><compound-statement>
    //<parameter-clause> ::=  '(' [<parameter-declaration-list>] ')'
    //<parameter-declaration-list> ::= <parameter-declaration>{','<parameter-declaration>}
    std::optional<CompilationError> Analyser::analyseFunctionDefinition() {
        area="global";
        auto next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::INT && next.value().GetType() != TokenType::VOID)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);


        //加指令 的多参数 修改指令的新建、输出等？ UNsure
        auto type = next.value().GetType();

        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
        //作用域判断 UNsure
        if (isLocal(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);

        auto fun = next.value();

        if (fun.GetValueString().compare("main") == 0)
            nymain = true;

        //fun不能被当做变量使用 undone
        addConstant(fun, fun.GetValueString());
        //函数表存储了函数的名字、参数占用的slot数、函数嵌套层级。
        //加函数表 unsure 层级
        //----------------do sth
        Function_info funcs{getIndex(fun.GetValueString()), 1};
        funcv.push_back(funcs);
        addFunction(fun);

        area = fun.GetValueString();

        int slot = 0;
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
        next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
        if (next.value().GetType() == TokenType::CONST || next.value().GetType() == TokenType::INT) {
            unreadToken();

            //参数的表切换？  undone
            auto err = analyseParameterDeclaration();
            if (err.has_value())
                return err;
            slot++;
            while (true) {
                next = nextToken();
                if (!next.has_value())
                    break;
                if (next.value().GetType() != TokenType::COMMA) {
                    unreadToken();
                    break;
                }
                err = analyseParameterDeclaration();
                if (err.has_value())
                    return err;
                slot++;
            }
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);

        } else if (next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);


        funcv.at(getFunctionIndex(area)).params_size = slot;
        funcv.at(getFunctionIndex(area)).type = type;


        auto err = analyseCompoundStatement();
        if (err.has_value())
            return err;

        // if语句中iret其他没有返回?  undone
        //------------maybe do sth
        auto &ins = funcv.at(getFunctionIndex(area))._instructions;
        if (type == TokenType::INT) {
            //bool ret = false;
            for (long long unsigned int i = 0; i < ins.size(); i++) {
                if (ins.at(i).GetOperation() == Operation::ret)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidReturn);
                //if (ins.at(i).GetOperation() == Operation::iret)
                    //ret = true;
            }
            //if (!ret)
            //    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidReturn);
            ins.emplace_back(Operation::iret);
        } else if (type == TokenType::VOID) {
            for (long long unsigned int i = 0; i < ins.size(); i++) {
                if (ins.at(i).GetOperation() == Operation::iret)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidReturn);
            }
            ins.emplace_back(Operation::ret);
        }

        return {};
    }

    //<parameter-declaration> ::=  [<const-qualifier>]<type-specifier><identifier>
    std::optional<CompilationError> Analyser::analyseParameterDeclaration() {
        auto next = nextToken();
        if (!next.has_value() ||
            (next.value().GetType() != TokenType::INT && next.value().GetType() != TokenType::CONST))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
        auto type = next.value().GetType();
        if (type == TokenType::CONST) {
            //加入自己const表，新index
            next = nextToken();
            if (!next.has_value() && (next.value().GetType() != TokenType::INT))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
            next = nextToken();
            if (!next.has_value() && (next.value().GetType() != TokenType::IDENTIFIER))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
            funcv.at(getFunctionIndex(area))._consts[next.value().GetValueString()] = funcv.at(
                    getFunctionIndex(area))._nextTokenIndex++;

        } else if (type == TokenType::INT) {
            next = nextToken();
            if (!next.has_value() && (next.value().GetType() != TokenType::IDENTIFIER))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
            funcv.at(getFunctionIndex(area))._vars[next.value().GetValueString()] = funcv.at(
                    getFunctionIndex(area))._nextTokenIndex++;
        }
        return {};
    }

    //<compound-statement>::='{' {<variable-declaration>} <statement-seq> '}'
    std::optional<CompilationError> Analyser::analyseCompoundStatement() {
        auto next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACE)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
        while (true) {
            next = nextToken();
            if (!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
            if (next.value().GetType() == TokenType::CONST) {
                unreadToken();
                auto err = analyseConstantDeclaration();
                if (err.has_value())
                    return err;
            } else if (next.value().GetType() == TokenType::INT) {
                unreadToken();
                auto err = analyseVariableDeclaration();
                if (err.has_value())
                    return err;
            } else {
                unreadToken();
                break;
            }
        }
        auto err = analyseStatementSequence();
        if (err.has_value())
            return err;
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACE)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteFunctionDefinition);
        return {};
    }


    //<statement-seq> ::= {<statement>}
    std::optional<CompilationError> Analyser::analyseStatementSequence() {
        while (true) {
            auto next = nextToken();
            if (!next.has_value())
                return {};
            unreadToken();
            if (next.value().GetType() != TokenType::IDENTIFIER &&
                next.value().GetType() != TokenType::PRINT &&
                next.value().GetType() != TokenType::IF &&
                next.value().GetType() != TokenType::LEFT_BRACE &&
                next.value().GetType() != TokenType::WHILE &&
                next.value().GetType() != TokenType::SCAN &&
                next.value().GetType() != TokenType::SEMICOLON &&
                next.value().GetType() != TokenType::RETURN) {
                return {};
            }
            auto err = analyseStatement();
            if (err.has_value())
                return err;
        }
        return {};
    }

    //<statement> ::=
    //      '{' <statement-seq> '}'-------------{
    //    |<condition-statement>------------if else  switch扩展
    //    |<loop-statement>-----------------while  其余扩展
    //    |<jump-statement>-----------------return  其余扩展
    //    |<print-statement>----------------print
    //    |<scan-statement>-----------------scan
    //    |<assignment-expression>';'-------identifier=
    //    |<function-call>';'---------------identifier(
    //    |';'------------------------------;
    std::optional<CompilationError> Analyser::analyseStatement() {
        auto next = nextToken();
        unreadToken();
        std::optional<CompilationError> err;
        switch (next.value().GetType()) {
            case TokenType::IDENTIFIER:
                nextToken();
                next = nextToken();
                if (!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos,
                                                                ErrorCode::ErrIncompleteFunctionDefinition);
                if (next.value().GetType() == TokenType::LEFT_BRACKET) {
                    unreadToken();
                    unreadToken();
                    err = analyseFunctionCall();
                    if (err.has_value())
                        return err;
                } else if (next.value().GetType() == TokenType::EQUAL_SIGN) {
                    unreadToken();
                    unreadToken();
                    err = analyseAssignmentStatement();
                    if (err.has_value())
                        return err;
                } else {
                    unreadToken();
                    unreadToken();
                    return {};
                }
                next = nextToken();
                if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
                    return std::make_optional<CompilationError>(_current_pos,
                                                                ErrorCode::ErrNoSemicolon);
                break;
            case TokenType::PRINT:
                err = analysePrintStatement();
                if (err.has_value())
                    return err;
                break;
            case TokenType::SCAN:
                err = analyseScanStatement();
                if (err.has_value())
                    return err;
                break;
            case TokenType::IF:
                err = analyseConditionStatement();
                if (err.has_value())
                    return err;
                break;
            case TokenType::LEFT_BRACE:
                nextToken();
                err = analyseStatementSequence();
                if (err.has_value())
                    return err;
                next = nextToken();
                if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACE)
                    return std::make_optional<CompilationError>(_current_pos,
                                                                ErrorCode::ErrInvalidStatement);
                break;
            case TokenType::WHILE:
                err = analyseLoopStatement();
                if (err.has_value())
                    return err;
                break;
            case TokenType::SEMICOLON:
                nextToken();
                break;
            case TokenType::RETURN:
                err = analyseJumpStatement();
                if (err.has_value())
                    return err;
                break;
            default:
                break;
        }

        return {};
    }

    //<jump-statement> ::= <return-statement>
    //<return-statement> ::= 'return' [<expression>] ';'
    std::optional<CompilationError> Analyser::analyseJumpStatement() {
        auto next = nextToken();
        next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos,
                                                        ErrorCode::ErrNoSemicolon);
        if (next.value().GetType() != TokenType::SEMICOLON) {
            unreadToken();
            auto err = analyseExpression();
            if (err.has_value())
                return err;
            funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::iret);
            next = nextToken();
        } else
            funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::ret);
        if (next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos,
                                                        ErrorCode::ErrNoSemicolon);
        return {};
        //指令 unsure
        //各种表维护 跳转call之后
        //if (area.compare("global") == 0)
        //不应该出现 unure
        //    return std::make_optional<CompilationError>(_current_pos,
        //                                                ErrorCode::ErrJumpStatement);

    }

    //<condition-statement> ::=
    //     'if' '(' <condition> ')' <statement> ['else' <statement>]
    //    |'switch' '(' <expression> ')' '{' {<labeled-statement>} '}'   扩展
    //<condition> ::= <expression>[<relational-operator><expression>]
    std::optional<CompilationError> Analyser::analyseConditionStatement() {
        auto next = nextToken();
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);

        auto err = analyseExpression();
        if (err.has_value())
            return err;

        next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);

        int jmp;
        if (next.value().GetType() == LESS_SIGN || next.value().GetType() == LESS_EQUAL_SIGN ||
            next.value().GetType() == MORE_SIGN || next.value().GetType() == MORE_EQUAL_SIGN ||
            next.value().GetType() == NOT_EQUAL_SIGN || next.value().GetType() == EQUAL_EQUAL_SIGN) {
            auto op = next.value().GetType();

            err = analyseExpression();
            if (err.has_value())
                return err;

            //指令？unsure
            funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::icmp);

            jmp = funcv.at(getFunctionIndex(area))._instructions.size();
            switch (op) {
                case TokenType::LESS_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jge);//加offset undone
                    break;
                case TokenType::LESS_EQUAL_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jg);//加offset undone
                    break;
                case TokenType::MORE_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jle);//加offset undone
                    break;
                case TokenType::MORE_EQUAL_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jl);//加offset undone
                    break;
                case TokenType::EQUAL_EQUAL_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jne);//加offset undone
                    break;
                case TokenType::NOT_EQUAL_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::je);//加offset undone
                    break;
                default:
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);
            }

            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);
        } else if (next.value().GetType() == TokenType::RIGHT_BRACKET) {
            jmp = funcv.at(getFunctionIndex(area))._instructions.size();
            funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::je);
        } else
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);


        //true false  跳转指令？ undone

        err = analyseStatement();
        if (err.has_value())
            return err;

        //修改跳转指令 unsure
        auto &ins = funcv.at(getFunctionIndex(area))._instructions;
        int offset = ins.size();
        //ins.emplace(ins[jmp],);
        ins.at(jmp).SetX(offset);
        //auto &j = ins.at(jmp);
        //std::vector<Instruction> tmpinsv;
        //j = tmpinsv.emplace_back(ins.at(jmp).GetOperation(), offset);


        next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::ELSE) {
            unreadToken();
            return {};
        }


        err = analyseStatement();
        if (err.has_value())
            return err;

        return {};
    }

    //loop-statement> ::=
    //    'while' '(' <condition> ')' <statement>
    //   |'do' <statement> 'while' '(' <condition> ')' ';'  扩展
    //   |'for' '('<for-init-statement> [<condition>]';' [<for-update-expression>]')' <statement> 扩展
    std::optional<CompilationError> Analyser::analyseLoopStatement() {
        auto next = nextToken();
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);

        int loop = funcv.at(getFunctionIndex(area))._instructions.size();
        auto err = analyseExpression();
        if (err.has_value())
            return err;

        next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);

        int jmp;
        if (next.value().GetType() == LESS_SIGN || next.value().GetType() == LESS_EQUAL_SIGN ||
            next.value().GetType() == MORE_SIGN || next.value().GetType() == MORE_EQUAL_SIGN ||
            next.value().GetType() == NOT_EQUAL_SIGN || next.value().GetType() == EQUAL_EQUAL_SIGN) {
            auto op = next.value().GetType();

            err = analyseExpression();
            if (err.has_value())
                return err;

            //指令？unsure
            funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::icmp);

            jmp = funcv.at(getFunctionIndex(area))._instructions.size();
            switch (op) {
                case TokenType::LESS_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jge);//加offset undone
                    break;
                case TokenType::LESS_EQUAL_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jg);//加offset undone
                    break;
                case TokenType::MORE_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jle);//加offset undone
                    break;
                case TokenType::MORE_EQUAL_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jl);//加offset undone
                    break;
                case TokenType::EQUAL_EQUAL_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::jne);//加offset undone
                    break;
                case TokenType::NOT_EQUAL_SIGN:
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::je);//加offset undone
                    break;
                default:
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);
            }

            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);
        } else if (next.value().GetType() == TokenType::RIGHT_BRACKET) {
            jmp = funcv.at(getFunctionIndex(area))._instructions.size();
            funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::je);
        } else
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidConditionStatement);

        //true false  跳转指令？ undone
        err = analyseStatement();
        if (err.has_value())
            return err;

        //执行结束之后跳转回循环？  undone
        //-------------do sth
        auto &ins = funcv.at(getFunctionIndex(area))._instructions;
        ins.emplace_back(Operation::jmp, loop);
        int offset = ins.size();
        ins.at(jmp).SetX(offset);
        //ins.emplace(ins[jmp],);
        //auto &j = ins.at(jmp);
        //std::vector<Instruction> tmpinsv;
        //j = tmpinsv.emplace_back(ins.at(jmp).GetOperation(), offset);

        return {};

    }

    //<function-call> ::= <identifier> '(' [<expression-list>] ')'
    //<expression-list> ::= <expression>{','<expression>}
    std::optional<CompilationError> Analyser::analyseFunctionCall() {
        auto next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
        if (isLocal(next.value().GetValueString()) && area.compare("global") != 0)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
        auto fun = next.value();

        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);

        int paranum = 0;

        next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
        if (next.value().GetType() == TokenType::RIGHT_BRACKET) {
            //func call 指令 unsure
            if (area.compare("global") == 0)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
            if (paranum != funcv.at(getFunctionIndex(fun.GetValueString())).params_size)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
            if (!isFunction(fun.GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
            funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::call);

        } else {
            unreadToken();
            auto err = analyseExpression();
            if (err.has_value())
                return err;
            paranum++;
            //加载参数指令 undone

            while (true) {
                next = nextToken();
                if (!next.has_value())
                    break;
                if (next.value().GetType() != TokenType::COMMA) {
                    unreadToken();
                    break;
                }
                err = analyseExpression();
                if (err.has_value())
                    return err;
                paranum++;
                //加载参数指令 undone
            }
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);

            //func call 指令 unsure
            if (area.compare("global") == 0)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
            if (paranum != funcv.at(getFunctionIndex(fun.GetValueString())).params_size)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
            if (!isFunction(fun.GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
            funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::call,
                                                                        getFunctionIndex(fun.GetValueString()));
        }
        return {};
    }

    //<assignment-expression> ::=<identifier><assignment-operator><expression>
    std::optional<CompilationError> Analyser::analyseAssignmentStatement() {
        // 这里除了语法分析以外还要留意
        // 标识符声明过吗？
        // 标识符是常量吗？
        // 需要生成指令吗？
        auto next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidAssignment);
        if (!isDeclared(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrNotDeclared);
        if (isConstant(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrAssignToConstant);
        if (isFunction(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrInvalidFunctionCall);
        auto var = next.value();

        //找到变量的 作用域，偏移
        auto &fun = funcv.at(getFunctionIndex(area));
        if (isLocal(var.GetValueString())) {
            fun._instructions.emplace_back(loada, 0, getIndex(var.GetValueString()));
        } else {
            fun._instructions.emplace_back(loada, 1, getIndex(var.GetValueString()));
        }


        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN)
            return std::make_optional<CompilationError>(_current_pos, ErrInvalidAssignment);


        //expression中获得具体指，压栈
        auto err = analyseExpression();
        if (err.has_value())
            return err;

        ///////////////////未赋值的放到赋值过的
        if (!isInitializedVariable(var.GetValueString())) {
            //不用从未初始化中删除
            //_uninitialized_vars.erase(next.value().GetValueString());
            //寻址是否正确？ undone
            if(isLocal(var.GetValueString()))
                fun._vars[var.GetValueString()] = fun._nextTokenIndex++;
            else
                _vars[var.GetValueString()] = _nextTokenIndex++;
        }

        //store到位置 先弹值 再弹地址 undone
        //变量的类型 undone
        fun._instructions.emplace_back(Operation::istore);

        return {};
    }

    //<scan-statement> ::='scan' '(' <identifier> ')' ';'
    std::optional<CompilationError> Analyser::analyseScanStatement() {
        auto next = nextToken();
        // '('
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidScan);
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
        if (!isDeclared(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
        if (isConstant(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);

        auto var = next.value().GetValueString();
        auto &fun = funcv.at(getFunctionIndex(area));
        if (isLocal(var)) {
            fun._instructions.emplace_back(loada, 0, getIndex(var));
        } else {
            fun._instructions.emplace_back(loada, 1, getIndex(var));
        }
        if (!isInitializedVariable(var)) {
            //不用从未初始化中删除
            //_uninitialized_vars.erase(next.value().GetValueString());
            //寻址是否正确？ undone
            if(isLocal(var))
                fun._vars[var] = fun._nextTokenIndex++;
            else
                _vars[var] = _nextTokenIndex++;
        }

        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidScan);
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        //-----------do sth
        //从标准输入根据一定格式解析字符，并压栈解析得到的值value
        //通过iload istore保存到变量地址？ unsure
        fun._instructions.emplace_back(Operation::iscan);
        //此时立即istore？  unsure
        fun._instructions.emplace_back(Operation::istore);

        return {};
    }

    // <print-statement>::= 'print' '(' [<printable-list>] ')' ';'
    std::optional<CompilationError> Analyser::analysePrintStatement() {
        // 如果之前 <语句序列> 的实现正确，这里第一个 next 一定是 TokenType::PRINT
        auto next = nextToken();

        // '('
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

        // 扩展string需修改
        auto err = analysePrintableList();
        if (err.has_value())
            return err;

        // ')'
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

        // ';'
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        // 获得 expression的 值&类型 undone
        auto &ins = funcv.at(getFunctionIndex(area))._instructions;
        ins.emplace_back(Operation::bipush, 10);
        ins.emplace_back(Operation::cprint);
        return {};
    }

    //<printable-list>  ::=<printable> {',' <printable>}
    //<printable>::=    <expression> ---------------(type  +-  (expression  id  digit ' float funcall(id'(')
    //                  |<string-literal>-----------"  扩展
    //undone
    std::optional<CompilationError> Analyser::analysePrintableList() {
        auto err = analyseExpression();
        if (err.has_value())
            return err;
        auto &ins = funcv.at(getFunctionIndex(area))._instructions;
        ins.emplace_back(Operation::iprint);

        //加入打印内容 undone
        //此处还是expr里？  unsure

        while (true) {
            auto next = nextToken();
            if (!next.has_value())
                return {};
            if (next.value().GetType() != TokenType::COMMA) {
                unreadToken();
                return {};
            }
            ins.emplace_back(Operation::bipush, 32);
            ins.emplace_back(Operation::cprint);
            err = analyseExpression();
            if (err.has_value())
                return err;
            ins.emplace_back(Operation::iprint);

        }
    }

    //<expression> ::=<additive-expression>
    //<additive-expression> ::= <multiplicative-expression>
    //                          {<additive-operator><multiplicative-expression>}
    std::optional<CompilationError> Analyser::analyseExpression() {
        auto err = analyseMultiplicativeExpression();
        if (err.has_value())
            return err;

        while (true) {
            auto next = nextToken();
            if (!next.has_value())
                return {};

            auto type = next.value().GetType();
            if (type != TokenType::PLUS_SIGN && type != TokenType::MINUS_SIGN) {
                unreadToken();
                return {};
            }

            err = analyseMultiplicativeExpression();
            if (err.has_value())
                return err;

            if (type == TokenType::PLUS_SIGN) {
                if (area.compare("global") == 0)
                    _instructions.emplace_back(Operation::iadd);
                else
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::iadd);
            } else if (type == TokenType::MINUS_SIGN) {
                if (area.compare("global") == 0)
                    _instructions.emplace_back(Operation::isub);
                else
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::isub);
            }

        }
    }

    //<multiplicative-expression> ::= <unary-expression>{<multiplicative-operator><unary-expression>}
    std::optional<CompilationError> Analyser::analyseMultiplicativeExpression() {
        auto err = analyseUnaryExpression();
        if (err.has_value())
            return err;

        while (true) {
            auto next = nextToken();
            if (!next.has_value())
                return {};

            auto type = next.value().GetType();
            if (type != TokenType::MULTIPLICATION_SIGN && type != TokenType::DIVISION_SIGN) {
                unreadToken();
                return {};
            }

            err = analyseUnaryExpression();
            if (err.has_value())
                return err;

            if (type == TokenType::MULTIPLICATION_SIGN) {
                if (area.compare("global") == 0)
                    _instructions.emplace_back(Operation::imul);
                else
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::imul);
            } else if (type == TokenType::DIVISION_SIGN) {
                if (area.compare("global") == 0)
                    _instructions.emplace_back(Operation::idiv);
                else
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::idiv);
            }

        }
    }

    //<cast-expression>::= {'('<type-specifier>')'}<unary-expression> 扩展
    //<unary-expression> ::= [<unary-operator>]<primary-expression>
    //<primary-expression> ::=
    //     '('<expression>')'-----------------(
    //    |<identifier>-----------------------id
    //    |<integer-literal>------------------deci\hexa
    //    |<function-call>--------------------id (
    std::optional<CompilationError> Analyser::analyseUnaryExpression() {
        auto next = nextToken();
        auto prefix = 1;
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
        if (next.value().GetType() == TokenType::PLUS_SIGN)
            prefix = 1;
        else if (next.value().GetType() == TokenType::MINUS_SIGN)
            prefix = -1;
        else
            unreadToken();


        // 预读
        next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
        std::optional<CompilationError> err;
        auto var = next.value();
        switch (next.value().GetType()) {
            case TokenType::IDENTIFIER:
                next = nextToken();
                if (next.value().GetType() == TokenType::LEFT_BRACKET) {
                    unreadToken();
                    unreadToken();
                    if (funcv.at(getFunctionIndex(var.GetValueString())).type == TokenType::VOID)
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidReturn);
                    auto err = analyseFunctionCall();
                    if (err.has_value())
                        return err;
                    break;
                }
                unreadToken();
                if (!isDeclared(var.GetValueString()))
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
                if (!isInitializedVariable(var.GetValueString()) && !isConstant(var.GetValueString()))
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
                if (isFunction(var.GetValueString()))
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionCall);
                //得到变量地址+压栈？  undone
                if (isLocal(var.GetValueString())) {
                    if (area.compare("global") == 0) {
                        _instructions.emplace_back(Operation::loada, 0, getIndex(var.GetValueString()));
                        _instructions.emplace_back(Operation::iload);
                    } else {
                        funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::loada, 0,
                                                                                    getIndex(var.GetValueString()));
                        funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::iload);
                    }
                } else {
                    if (area.compare("global") == 0) {
                        _instructions.emplace_back(Operation::loada, 1, getIndex(var.GetValueString()));
                        _instructions.emplace_back(Operation::iload);
                    } else {
                        funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::loada, 1,
                                                                                    getIndex(var.GetValueString()));
                        funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::iload);
                    }
                }
                break;
            case TokenType::DECIMAL:
                //指令？  undone
                //addConstant(var, std::stoi(var.GetValueString(), nullptr, 10));
                if (area.compare("global") == 0)
                    _instructions.emplace_back(Operation::ipush, std::stoi(var.GetValueString(), nullptr, 10));
                else
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::ipush,
                                                                                std::stoi(var.GetValueString(), nullptr,
                                                                                          10));
                break;
            case TokenType::HEXADECIMAL:
                //指令？  undone
                //addConstant(var, std::stoi(var.GetValueString(), nullptr, 16));
                if (area.compare("global") == 0)
                    _instructions.emplace_back(Operation::ipush, std::stoi(var.GetValueString(), nullptr, 16));
                else
                    funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::ipush,
                                                                                std::stoi(var.GetValueString(), nullptr,
                                                                                          16));
                break;
            case TokenType::LEFT_BRACKET:
                err = analyseExpression();
                if (err.has_value())
                    return err;
                next = nextToken();
                if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
                break;
            default:
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
        }

        // 取负 unsure
        if (prefix == -1) {
            if (area.compare("global") == 0)
                _instructions.emplace_back(Operation::ineg);
            else
                funcv.at(getFunctionIndex(area))._instructions.emplace_back(Operation::ineg);
        }
        return {};
    }


    std::optional<Token> Analyser::nextToken() {
        if (_offset == _tokens.size())
            return {};
        // 考虑到 _tokens[0..._offset-1] 已经被分析过了
        // 所以我们选择 _tokens[0..._offset-1] 的 EndPos 作为当前位置
        _current_pos = _tokens[_offset].GetEndPos();
        auto next=_tokens[_offset++];
        while (next.GetType()==TokenType::COMMENT){
            if (_offset == _tokens.size())
                return {};
            _current_pos = _tokens[_offset].GetEndPos();
            next=_tokens[_offset++];
        }
        return next;
    }

    void Analyser::unreadToken() {
        if (_offset == 0)
            DieAndPrint("analyser unreads token from the begining.");
        _current_pos = _tokens[_offset - 1].GetEndPos();
        _offset--;
    }

    void Analyser::_add(const Token &tk, std::map<std::string, int32_t> &mp) {
        if (tk.GetType() != TokenType::IDENTIFIER)
            DieAndPrint("only identifier can be added to the table.");
        mp[tk.GetValueString()] = _nextTokenIndex;
        _nextTokenIndex++;
    }

    void Analyser::addVariable(const Token &tk) {
        _add(tk, _vars);
    }

    void Analyser::addConstant(const Token &tk, std::string value) {
        //if (tk.GetType() != TokenType::IDENTIFIER)
        //    DieAndPrint("only identifier can be added to the table.");
        _consts[tk.GetValueString()] = constv.size();
        Constant_info con{0, value};
        constv.push_back(con);
        //_nextTokenIndex++;
    }

    void Analyser::addConstant(const Token &tk, int value) {
        //if (tk.GetType() != TokenType::IDENTIFIER)
        //    DieAndPrint("only identifier can be added to the table.");
        _consts[tk.GetValueString()] = constv.size();
        Constant_info con{1, value};
        constv.push_back(con);
        //_nextTokenIndex++;
    }

    void Analyser::addConstant(const Token &tk) {
        _add(tk, _consts);
    }

    void Analyser::addFunction(const Token &tk) {
        if (tk.GetType() != TokenType::IDENTIFIER)
            DieAndPrint("only identifier can be added to the table.");
        _funcs[tk.GetValueString()] = funcv.size() - 1;
    }

    void Analyser::addUninitializedVariable(const Token &tk) {
        _add(tk, _uninitialized_vars);
    }

    int32_t Analyser::getIndex(const std::string &s) {
        if (isLocal(s) && area.compare("global") != 0) {
            auto &fun = funcv.at(getFunctionIndex(area));
            if (fun._uninitialized_vars.find(s) != fun._uninitialized_vars.end())
                return fun._uninitialized_vars[s];
            else if (fun._vars.find(s) != fun._vars.end())
                return fun._vars[s];
            else
                return fun._consts[s];
        }
        if (_uninitialized_vars.find(s) != _uninitialized_vars.end())
            return _uninitialized_vars[s];
        else if (_vars.find(s) != _vars.end())
            return _vars[s];
        else
            return _consts[s];
    }

    int32_t Analyser::getFunctionIndex(const std::string &s) {
        if (_funcs.find(s) != _funcs.end())
            return _funcs[s];
    }

    bool Analyser::isLocal(const std::string &s) {
        if (area.compare("global") != 0 && (
                funcv.at(getFunctionIndex(area))._vars.find(s) != funcv.at(getFunctionIndex(area))._vars.end() ||
                funcv.at(getFunctionIndex(area))._uninitialized_vars.find(s) !=
                funcv.at(getFunctionIndex(area))._uninitialized_vars.end() ||
                funcv.at(getFunctionIndex(area))._consts.find(s) != funcv.at(getFunctionIndex(area))._consts.end()))
            return true;
        else if (area.compare("global") == 0 && (
                _vars.find(s) != _vars.end() ||
                _uninitialized_vars.find(s) != _uninitialized_vars.end() ||
                _consts.find(s) != _consts.end() || _funcs.find(s) != _funcs.end()))
            return true;
        return false;
    }

    bool Analyser::isDeclared(const std::string &s) {
        return isConstant(s) || isUninitializedVariable(s) || isInitializedVariable(s);
    }

    bool Analyser::isUninitializedVariable(const std::string &s) {
        if (area.compare("global") != 0 &&
            funcv.at(getFunctionIndex(area))._uninitialized_vars.find(s) !=
            funcv.at(getFunctionIndex(area))._uninitialized_vars.end())
            return true;
        return _uninitialized_vars.find(s) != _uninitialized_vars.end();
    }

    bool Analyser::isInitializedVariable(const std::string &s) {
        if (area.compare("global") != 0) {
            if (funcv.at(getFunctionIndex(area))._vars.find(s) != funcv.at(getFunctionIndex(area))._vars.end())
                return true;
            if (isLocal(s))
                return false;
        }
        return _vars.find(s) != _vars.end();
    }

    bool Analyser::isConstant(const std::string &s) {
        if (area.compare("global") == 0)
            return _consts.find(s) != _consts.end();
        else {
            if (funcv.at(getFunctionIndex(area))._consts.find(s) != funcv.at(getFunctionIndex(area))._consts.end())
                return true;
            else {
                if (isLocal(s))
                    return false;
                else
                    return _consts.find(s) != _consts.end();
            }
        }
    }

    bool Analyser::isFunction(const std::string &s) {
        if (_funcs.find(s) != _funcs.end())
            return true;
        return false;
    }

}