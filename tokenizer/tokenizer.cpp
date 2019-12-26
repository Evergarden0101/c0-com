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

    // ע�⣺����ķ���ֵ�� Token �� CompilationError ֻ�ܷ���һ��������ͬʱ���ء�
    std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::nextToken() {
        // ���ڴ洢�Ѿ���������ɵ�ǰtoken�ַ�
        std::stringstream ss;
        // ����token�Ľ������Ϊ�˺����ķ���ֵ
        std::pair<std::optional<Token>, std::optional<CompilationError>> result;
        // <�кţ��к�>����ʾ��ǰtoken�ĵ�һ���ַ���Դ�����е�λ��
        std::pair<int64_t, int64_t> pos;
        // ��¼��ǰ�Զ�����״̬������˺���ʱ�ǳ�ʼ״̬
        DFAState current_state = DFAState::INITIAL_STATE;
        // ����һ����ѭ����������������
        // ÿһ��ִ��while�ڵĴ��룬�����ܵ���״̬�ı��
        while (true) {
            // ��һ���ַ�����ע��auto�Ƶ��ó���������std::optional<char>
            // ������ʵ������д��
            // 1. ÿ��ѭ��ǰ��������һ�� char
            // 2. ֻ���ڿ��ܻ�ת�Ƶ�״̬����һ�� char
            // ��Ϊ����ʵ���� unread��Ϊ��ʡ������ѡ���һ��
            auto current_char = nextChar();
            // ��Ե�ǰ��״̬���в�ͬ�Ĳ���
            switch (current_state) {

                // ��ʼ״̬
                // ��� case ���Ǹ����˺����߼������Ǻ���� case �����հᡣ
                case INITIAL_STATE: {
                    // �Ѿ��������ļ�β
                    if (!current_char.has_value())
                        // ����һ���յ�token���ͱ������ErrEOF���������ļ�β
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(0, 0, ErrEOF));

                    // ��ȡ�������ַ���ֵ��ע��auto�Ƶ�����������char
                    auto ch = current_char.value();
                    // ����Ƿ�����˲��Ϸ����ַ�����ʼ��Ϊ��
                    auto invalid = false;

                    // ʹ�����Լ���װ���ж��ַ����͵ĺ����������� tokenizer/utils.hpp
                    // see https://en.cppreference.com/w/cpp/string/byte/isblank
                    if (miniplc0::isspace(ch)) // �������ַ��ǿհ��ַ����ո񡢻��С��Ʊ���ȣ�
                        current_state = DFAState::INITIAL_STATE; // ������ǰ״̬Ϊ��ʼ״̬���˴�ֱ��breakҲ�ǿ��Ե�
                    else if (!miniplc0::isprint(ch)) // control codes and backspace
                        invalid = true;
                    else if (miniplc0::isdigit(ch)) // �������ַ�������
                        current_state = DFAState::DECIMAL_STATE; // �л����޷���������״̬
                    else if (miniplc0::isalpha(ch)) // �������ַ���Ӣ����ĸ
                        current_state = DFAState::IDENTIFIER_STATE; // �л�����ʶ����״̬
                    else {
                        switch (ch) {
                            case '=': // ����������ַ���`=`�����л������ںŵ�״̬
                                current_state = DFAState::EQUAL_SIGN_STATE;
                                break;
                            case '-':
                                // ����գ��л������ŵ�״̬
                                current_state = DFAState::MINUS_SIGN_STATE;
                                break;
                            case '+':
                                // ����գ��л����Ӻŵ�״̬
                                current_state = DFAState::PLUS_SIGN_STATE;
                                break;
                            case '*':
                                // ����գ��л�״̬
                                current_state = DFAState::MULTIPLICATION_SIGN_STATE;
                                break;
                            case '/':
                                // ����գ��л�״̬
                                current_state = DFAState::DIVISION_SIGN_STATE;
                                break;

                                ///// ����գ�
                                ///// ���������Ŀɽ����ַ�
                                ///// �л�����Ӧ��״̬
                            case '(':
                                current_state = DFAState::LEFTBRACKET_STATE;
                                break;
                            case ')':
                                current_state = DFAState::RIGHTBRACKET_STATE;
                                break;
                            case ';':
                                current_state = DFAState::SEMICOLON_STATE;
                                break;

                                //C0 unsure, {<= != ==}  �����
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
                                // �����ܵ��ַ����µĲ��Ϸ���״̬
                            default:
                                invalid = true;
                                break;
                        }
                    }
                    // ����������ַ�������״̬��ת�ƣ�˵������һ��token�ĵ�һ���ַ�
                    if (current_state != DFAState::INITIAL_STATE)
                        pos = previousPos(); // ��¼���ַ��ĵ�λ��Ϊtoken�Ŀ�ʼλ��
                    // �����˲��Ϸ����ַ�
                    if (invalid) {
                        // ��������ַ�
                        unreadLast();
                        // ���ر�����󣺷Ƿ�������
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
                    }
                    // ����������ַ�������״̬��ת�ƣ�˵������һ��token�ĵ�һ���ַ�
                    if (current_state != DFAState::INITIAL_STATE) {// ignore white spaces
                        ss << ch; // �洢�������ַ�

                        //C0 ����0x��0 ʮ��������ʮ����
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

                    //С��
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

                    //����
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

                    //����
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

                    //C0 ,10������16����  0��0x��1234...
                    // ��ǰ״̬���޷�������
                case DECIMAL_STATE: {
                    // �����ǰ�Ѿ��������ļ�β��������Ѿ��������ַ���Ϊ����
                    //     �����ɹ��򷵻��޷����������͵�token�����򷵻ر������
                    if (!current_char.has_value()) {
                        std::string s;
                        ss >> s;
                        return std::make_pair(
                                std::make_optional<Token>(TokenType::DECIMAL, s, pos, currentPos()),
                                std::optional<CompilationError>());

                    }
                        // ����������ַ������֣���洢�������ַ�
                    else if (isdigit(current_char.value())) {
                        ss << current_char.value();
                    }
                        // �������������ĸ����洢�������ַ������л�״̬����ʶ��
                    else if (isalpha(current_char.value())) {
                        ss << current_char.value();
                        current_state = DFAState::IDENTIFIER_STATE;
                    }
                        // ����������ַ������������֮һ������˶������ַ����������Ѿ��������ַ���Ϊ����
                        //     �����ɹ��򷵻��޷����������͵�token�����򷵻ر������
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
                        // ����������ַ������ֺ�ABCDEF����洢�������ַ�
                    else if ((isdigit(current_char.value())) || isHexa(current_char.value())) {
                        ss << current_char.value();
                    }
                        // �������������ĸ����洢�������ַ������л�״̬����ʶ��
                    else if (isalpha(current_char.value()) && !isHexa(current_char.value())) {
                        ss << current_char.value();
                        current_state = DFAState::IDENTIFIER_STATE;
                    }
                        // ����������ַ������������֮һ������˶������ַ����������Ѿ��������ַ���Ϊ����
                        //     �����ɹ��򷵻��޷����������͵�token�����򷵻ر������
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
                    // ����գ�
                    // �����ǰ�Ѿ��������ļ�β��������Ѿ��������ַ���
                    //     �����������ǹؼ��֣���ô���ض�Ӧ�ؼ��ֵ�token�����򷵻ر�ʶ����token

                    if (!current_char.has_value()) {
                        std::string s;
                        ss >> s;
                        return checkReserved(s, pos);
                    }
                        // ������������ַ�����ĸ����洢�������ַ�
                    else if (isalnum(current_char.value())) {
                        ss << current_char.value();
                    }
                        // ����������ַ������������֮һ������˶������ַ����������Ѿ��������ַ���
                        //     �����������ǹؼ��֣���ô���ض�Ӧ�ؼ��ֵ�token�����򷵻ر�ʶ����token
                    else {
                        unreadLast();
                        std::string s;
                        ss >> s;
                        return checkReserved(s, pos);
                    }
                    break;
                }

                    // �����ǰ״̬�ǼӺ�
                case PLUS_SIGN_STATE: {
                    // ��˼������ΪʲôҪ���ˣ��������ط��᲻����Ҫ
                    unreadLast(); // Yes, we unread last char even if it's an EOF.
                    return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN, '+', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                    // ��ǰ״̬Ϊ���ŵ�״̬
                case MINUS_SIGN_STATE: {
                    // ����գ����ˣ������ؼ���token
                    unreadLast(); // Yes, we unread last char even if it's an EOF.
                    return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN, '-', pos, currentPos()),
                                          std::optional<CompilationError>());
                }

                    // ����գ�
                    // ���������ĺϷ�״̬�����к��ʵĲ���
                    // ������н���������token�����ر������
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
                    //������ �� ��ֵ
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

                    // Ԥ��֮���״̬�����ִ�е������˵�������쳣
                default:
                    DieAndPrint("unhandled state.");
                    break;
            }
        }
        // Ԥ��֮���״̬�����ִ�е������˵�������쳣
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
        //�ؼ��� 'const'
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