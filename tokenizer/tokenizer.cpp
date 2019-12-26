#include "tokenizer/tokenizer.h"

#include <cctype>
#include <sstream>

namespace C0 {

    std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::NextToken() {
        if (!_initialized)
            readAll();
        if (_rdr.bad())
            return std::make_pair(std::optional<Token>(),
                                  std::make_optional<CompilationError>(0, 0, ErrorCode::ErrStreamError));
        if (isEOF())
            return std::make_pair(std::optional<Token>(),
                                  std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));
        auto p = nextToken();
        if (p.second.has_value())
            return std::make_pair(p.first, p.second);
        auto err = checkToken(p.first.value());
        if (err.has_value())
            return std::make_pair(p.first, err.value());
        return std::make_pair(p.first, std::optional<CompilationError>());
    }

    std::pair<std::vector<Token>, std::optional<CompilationError>> Tokenizer::AllTokens() {
        std::vector<Token> result;
        while (true) {
            auto p = NextToken();
            if (p.second.has_value()) {
                if (p.second.value().GetCode() == ErrorCode::ErrEOF)
                    return std::make_pair(result, std::optional<CompilationError>());
                else
                    return std::make_pair(std::vector<Token>(), p.second);
            }
            result.emplace_back(p.first.value());
        }
    }

    // 注意：这里的返回值中 Token 和 CompilationError 只能返回一个，不能同时返回。
    std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::nextToken() {
        // 用于存储已经读到的组成当前token字符
        std::stringstream ss;
        // 分析token的结果，作为此函数的返回值
        std::pair<std::optional<Token>, std::optional<CompilationError>> result;
        // <行号，列号>，表示当前token的第一个字符在源代码中的位置
        std::pair<int64_t, int64_t> pos;
        // 记录当前自动机的状态，进入此函数时是初始状态
        DFAState current_state = DFAState::INITIAL_STATE;
        // 这是一个死循环，除非主动跳出
        // 每一次执行while内的代码，都可能导致状态的变更
        while (true) {
            // 读一个字符，请注意auto推导得出的类型是std::optional<char>
            // 这里其实有两种写法
            // 1. 每次循环前立即读入一个 char
            // 2. 只有在可能会转移的状态读入一个 char
            // 因为我们实现了 unread，为了省事我们选择第一种
            auto current_char = nextChar();
            // 针对当前的状态进行不同的操作
            switch (current_state) {

                // 初始状态
                // 这个 case 我们给出了核心逻辑，但是后面的 case 不用照搬。
                case INITIAL_STATE: {
                    // 已经读到了文件尾
                    if (!current_char.has_value())
                        // 返回一个空的token，和编译错误ErrEOF：遇到了文件尾
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(0, 0, ErrEOF));

                    // 获取读到的字符的值，注意auto推导出的类型是char
                    auto ch = current_char.value();
                    // 标记是否读到了不合法的字符，初始化为否
                    auto invalid = false;

                    // 使用了自己封装的判断字符类型的函数，定义于 tokenizer/utils.hpp
                    // see https://en.cppreference.com/w/cpp/string/byte/isblank
                    if (miniplc0::isspace(ch)) // 读到的字符是空白字符（空格、换行、制表符等）
                        current_state = DFAState::INITIAL_STATE; // 保留当前状态为初始状态，此处直接break也是可以的
                    else if (!miniplc0::isprint(ch)) // control codes and backspace
                        invalid = true;
                    else if (miniplc0::isdigit(ch)) // 读到的字符是数字
                        current_state = DFAState::DECIMAL_STATE; // 切换到无符号整数的状态
                    else if (miniplc0::isalpha(ch)) // 读到的字符是英文字母
                        current_state = DFAState::IDENTIFIER_STATE; // 切换到标识符的状态
                    else {
                        switch (ch) {
                            case '=': // 如果读到的字符是`=`，则切换到等于号的状态
                                current_state = DFAState::EQUAL_SIGN_STATE;
                                break;
                            case '-':
                                // 请填空：切换到减号的状态
                                current_state = DFAState::MINUS_SIGN_STATE;
                                break;
                            case '+':
                                // 请填空：切换到加号的状态
                                current_state = DFAState::PLUS_SIGN_STATE;
                                break;
                            case '*':
                                // 请填空：切换状态
                                current_state = DFAState::MULTIPLICATION_SIGN_STATE;
                                break;
                            case '/':
                                // 请填空：切换状态
                                current_state = DFAState::DIVISION_SIGN_STATE;
                                break;

                                ///// 请填空：
                                ///// 对于其他的可接受字符
                                ///// 切换到对应的状态
                            case '(':
                                current_state = DFAState::LEFTBRACKET_STATE;
                                break;
                            case ')':
                                current_state = DFAState::RIGHTBRACKET_STATE;
                                break;
                            case ';':
                                current_state = DFAState::SEMICOLON_STATE;
                                break;

                                //C0 unsure, {<= != ==}  的情况
                            case '{':
                                current_state = DFAState::LEFTBRACE_STATE;
                                break;
                            case '}':
                                current_state = DFAState::RIGHTBRACE_STATE;
                                break;
                            case ',':
                                current_state = DFAState::COMMA_STATE;
                                break;
                            case '!':
                                current_state = DFAState::NOT_EQUAL_STATE;
                                break;
                            case '<':
                                current_state = DFAState::LESS_STATE;
                                break;
                            case '>':
                                current_state = DFAState::MORE_STATE;
                                break;
                                // 不接受的字符导致的不合法的状态
                            default:
                                invalid = true;
                                break;
                        }
                    }
                    // 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
                    if (current_state != DFAState::INITIAL_STATE)
                        pos = previousPos(); // 记录该字符的的位置为token的开始位置
                    // 读到了不合法的字符
                    if (invalid) {
                        // 回退这个字符
                        unreadLast();
                        // 返回编译错误：非法的输入
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
                    }
                    // 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
                    if (current_state != DFAState::INITIAL_STATE) {// ignore white spaces
                        ss << ch; // 存储读到的字符

                        //C0 区分0x与0 十六进制与十进制
                        if (ch == '0') {
                            current_char = nextChar();
                            if (current_char.value() == 'x' || current_char.value() == 'X') {
                                current_state = DFAState::HEXADECIMAL_STATE;
                                ss << current_char.value();
                            } else if (isdigit(current_char.value())) {
                                return std::make_pair(std::optional<Token>(),
                                                      std::make_optional<CompilationError>(pos,
                                                                                           ErrorCode::ErrInvalidInput));
                            } else {
                                unreadLast();
                            }
                        }
                    }
                    break;
                }

                    //小于
                case LESS_STATE: {
                    if (!current_char.has_value()) {
                        unreadLast();
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::LESS_SIGN, '<', pos, currentPos()),
                                std::optional<CompilationError>());
                    } else if (current_char.value() == '=') {
                        std::string s = "<=";
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::LESS_EQUAL_SIGN, s, pos, currentPos()),
                                std::optional<CompilationError>());
                    } else {
                        unreadLast();
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::LESS_SIGN, '<', pos, currentPos()),
                                std::optional<CompilationError>());
                    }
                    break;
                }

                    //大于
                case MORE_STATE: {
                    if (!current_char.has_value()) {
                        unreadLast();
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::MORE_SIGN, '>', pos, currentPos()),
                                std::optional<CompilationError>());
                    } else if (current_char.value() == '=') {
                        std::string s = ">=";
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::MORE_EQUAL_SIGN, s, pos, currentPos()),
                                std::optional<CompilationError>());
                    } else {
                        unreadLast();
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::MORE_SIGN, '>', pos, currentPos()),
                                std::optional<CompilationError>());
                    }
                    break;
                }

                    //不等
                case NOT_EQUAL_STATE: {
                    if (!current_char.has_value()) {
                        unreadLast();
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(pos,
                                                                                   ErrorCode::ErrIncompleteExpression));
                    } else if (current_char.value() == '=') {
                        std::string s = "!=";
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::NOT_EQUAL_SIGN, s, pos, currentPos()),
                                std::optional<CompilationError>());
                    } else {
                        unreadLast();
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(pos,
                                                                                   ErrorCode::ErrIncompleteExpression));
                    }
                }

                    //C0 ,10进制与16进制  0与0x与1234...
                    // 当前状态是无符号整数
                case DECIMAL_STATE: {
                    // 如果当前已经读到了文件尾，则解析已经读到的字符串为整数
                    //     解析成功则返回无符号整数类型的token，否则返回编译错误
                    if (!current_char.has_value()) {
                        std::string s;
                        ss >> s;
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::DECIMAL, s, pos, currentPos()),
                                std::optional<CompilationError>());

                    }
                        // 如果读到的字符是数字，则存储读到的字符
                    else if (isdigit(current_char.value())) {
                        ss << current_char.value();
                    }
                        // 如果读到的是字母，则存储读到的字符，并切换状态到标识符
                    else if (isalpha(current_char.value())) {
                        ss << current_char.value();
                        current_state = DFAState::IDENTIFIER_STATE;
                    }
                        // 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串为整数
                        //     解析成功则返回无符号整数类型的token，否则返回编译错误
                    else {
                        unreadLast();
                        std::string s;
                        ss >> s;
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::DECIMAL, s, pos, currentPos()),
                                std::optional<CompilationError>());
                    }
                    break;
                }
                case HEXADECIMAL_STATE: {
                    if (!current_char.has_value()) {
                        std::string s;
                        ss >> s;
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::HEXADECIMAL, s, pos, currentPos()),
                                std::optional<CompilationError>());

                    }
                        // 如果读到的字符是数字和ABCDEF，则存储读到的字符
                    else if ((isdigit(current_char.value())) || isHexa(current_char.value())) {
                        ss << current_char.value();
                    }
                        // 如果读到的是字母，则存储读到的字符，并切换状态到标识符
                    else if (isalpha(current_char.value()) && !isHexa(current_char.value())) {
                        ss << current_char.value();
                        current_state = DFAState::IDENTIFIER_STATE;
                    }
                        // 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串为整数
                        //     解析成功则返回无符号整数类型的token，否则返回编译错误
                    else {
                        unreadLast();
                        std::string s;
                        ss >> s;
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::HEXADECIMAL, s, pos, currentPos()),
                                std::optional<CompilationError>());
                    }
                    break;
                }

                    //C0
                case IDENTIFIER_STATE: {
                    // 请填空：
                    // 如果当前已经读到了文件尾，则解析已经读到的字符串
                    //     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token

                    if (!current_char.has_value()) {
                        std::string s;
                        ss >> s;
                        return checkReserved(s, pos);
                    }
                        // 如果读到的是字符或字母，则存储读到的字符
                    else if (isalnum(current_char.value())) {
                        ss << current_char.value();
                    }
                        // 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串
                        //     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
                    else {
                        unreadLast();
                        std::string s;
                        ss >> s;
                        return checkReserved(s, pos);
                    }
                    break;
                }

                    // 如果当前状态是加号
                case PLUS_SIGN_STATE: {
                    // 请思考这里为什么要回退，在其他地方会不会需要
                    unreadLast(); // Yes, we unread last char even if it's an EOF.
                    return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN, '+', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                    // 当前状态为减号的状态
                case MINUS_SIGN_STATE: {
                    // 请填空：回退，并返回减号token
                    unreadLast(); // Yes, we unread last char even if it's an EOF.
                    return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN, '-', pos, currentPos()),
                                          std::optional<CompilationError>());
                }

                    // 请填空：
                    // 对于其他的合法状态，进行合适的操作
                    // 比如进行解析、返回token、返回编译错误
                case LEFTBRACKET_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACKET, '(', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case RIGHTBRACKET_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACKET, ')', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case SEMICOLON_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::SEMICOLON, ';', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case MULTIPLICATION_SIGN_STATE: {
                    unreadLast();
                    return std::make_pair(
                            std::make_optional<Token>(TokenType::MULTIPLICATION_SIGN, '*', pos, currentPos()),
                            std::optional<CompilationError>());
                }

                case MULTI_LINE_COMMENT: {
                    if (!current_char.has_value())
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));

                    if (current_char.value() == '/') {
                        std::string s;
                        ss >> s;
                        if (s.at(s.size() - 1) == '*') {
                            s.push_back('/');
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::COMMENT, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        }
                        ss.clear();
                        ss << s;
                        ss << current_char.value();
                    } else {
                        ss << current_char.value();
                    }
                    break;
                }

                case SINGLE_LINE_COMMENT: {
                    if (!current_char.has_value())
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));

                    if (current_char.value() == 10 || current_char.value() == 13) {
                        ss << current_char.value();
                        std::string s;
                        ss >> s;
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::COMMENT, s, pos, currentPos()),
                                std::optional<CompilationError>());
                    } else {
                        ss << current_char.value();
                    }
                    break;
                }

                case DIVISION_SIGN_STATE: {
                    if (current_char.value() == '*') {
                        ss << current_char.value();
                        current_state = DFAState::MULTI_LINE_COMMENT;
                        break;
                    } else if (current_char.value() == '/') {
                        ss << current_char.value();
                        current_state = DFAState::SINGLE_LINE_COMMENT;
                        break;
                    } else {
                        unreadLast();
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::DIVISION_SIGN, '/', pos, currentPos()),
                                std::optional<CompilationError>());
                    }
                }
                    //条件等 和 赋值
                case EQUAL_SIGN_STATE: {
                    if (!current_char.has_value()) {
                        unreadLast();
                        return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, '=', pos, currentPos()),
                                              std::optional<CompilationError>());
                    }
                    if (current_char.value() == '=') {
                        std::string s = "==";
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::EQUAL_EQUAL_SIGN, s, pos, currentPos()),
                                std::optional<CompilationError>());
                    } else {
                        unreadLast();
                        return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, '=', pos, currentPos()),
                                              std::optional<CompilationError>());
                    }
                }

                case LEFTBRACE_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACE, '{', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case RIGHTBRACE_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACE, '}', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case COMMA_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::COMMA, ',', pos, currentPos()),
                                          std::optional<CompilationError>());
                }

                    // 预料之外的状态，如果执行到了这里，说明程序异常
                default:
                    DieAndPrint("unhandled state.");
                    break;
            }
        }
        // 预料之外的状态，如果执行到了这里，说明程序异常
        return std::make_pair(std::optional<Token>(), std::optional<CompilationError>());
    }


    std::optional<CompilationError> Tokenizer::checkToken(const Token &t) {
        switch (t.GetType()) {
            case IDENTIFIER: {
                auto val = t.GetValueString();
                if (miniplc0::isdigit(val[0]))
                    return std::make_optional<CompilationError>(t.GetStartPos().first, t.GetStartPos().second,
                                                                ErrorCode::ErrInvalidIdentifier);
                break;
            }
            default:
                break;
        }
        return {};
    }

    void Tokenizer::readAll() {
        if (_initialized)
            return;
        for (std::string tp; std::getline(_rdr, tp);)
            _lines_buffer.emplace_back(std::move(tp + "\n"));
        _initialized = true;
        _ptr = std::make_pair<int64_t, int64_t>(0, 0);
        return;
    }

    // Note: We allow this function to return a postion which is out of bound according to the design like std::vector::end().
    std::pair<uint64_t, uint64_t> Tokenizer::nextPos() {
        if (_ptr.first >= _lines_buffer.size())
            DieAndPrint("advance after EOF");
        if (_ptr.second == _lines_buffer[_ptr.first].size() - 1)
            return std::make_pair(_ptr.first + 1, 0);
        else
            return std::make_pair(_ptr.first, _ptr.second + 1);
    }

    std::pair<uint64_t, uint64_t> Tokenizer::currentPos() {
        return _ptr;
    }

    std::pair<uint64_t, uint64_t> Tokenizer::previousPos() {
        if (_ptr.first == 0 && _ptr.second == 0)
            DieAndPrint("previous position from beginning");
        if (_ptr.second == 0)
            return std::make_pair(_ptr.first - 1, _lines_buffer[_ptr.first - 1].size() - 1);
        else
            return std::make_pair(_ptr.first, _ptr.second - 1);
    }

    std::optional<char> Tokenizer::nextChar() {
        if (isEOF())
            return {}; // EOF
        auto result = _lines_buffer[_ptr.first][_ptr.second];
        _ptr = nextPos();
        return result;
    }

    bool Tokenizer::isEOF() {
        return _ptr.first >= _lines_buffer.size();
    }

    // Note: Is it evil to unread a buffer?
    void Tokenizer::unreadLast() {
        _ptr = previousPos();
    }

    bool Tokenizer::isHexa(char c) {
        if ((c <= 'f' && c >= 'a') || (c <= 'F' && c >= 'A'))
            return true;
        else
            return false;
    }

    std::pair<std::optional<Token>, std::optional<CompilationError>>
    Tokenizer::checkReserved(std::string s, std::pair<int64_t, int64_t> pos) {
        //关键字 'const'
        //    |'void'   |'int'    |'char'   |'double'
        //    |'struct'
        //    |'if'     |'else'
        //    |'switch' |'case'   |'default'
        //    |'while'  |'for'    |'do'
        //    |'return' |'break'  |'continue'
        //    |'print'  |'scan'
        if (s.compare("void") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::VOID, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("int") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::INT, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("char") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::CHAR, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("double") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::DOUBLE, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("struct") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::STRUCT, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("if") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::IF, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("else") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::ELSE, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("switch") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::SWITCH, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("case") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::CASE, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("default") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::DEFAULT, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("int") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::INT, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("while") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::WHILE, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("for") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::FOR, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("DO") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::DO, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("return") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::RETURN, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("break") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::BREAK, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("continue") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::CONTINUE, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("int") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::INT, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("scan") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::SCAN, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("const") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::CONST, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else if (s.compare("print") == 0) {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::PRINT, s, pos, currentPos()),
                    std::optional<CompilationError>());
        } else {
            return std::make_pair(
                    std::make_optional<Token>(TokenType::IDENTIFIER, s, pos, currentPos()),
                    std::optional<CompilationError>());
        }
    }
}