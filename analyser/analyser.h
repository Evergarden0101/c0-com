#pragma once

#include "error/error.h"
#include "instruction/instruction.h"
#include "tokenizer/token.h"

#include <vector>
#include <optional>
#include <utility>
#include <map>
#include <cstdint>
#include <cstddef> // for std::size_t

namespace C0 {

	class Analyser final {
	private:
		using uint64_t = std::uint64_t;
		using int64_t = std::int64_t;
		using uint32_t = std::uint32_t;
		using int32_t = std::int32_t;
	public:
		Analyser(std::vector<Token> v)
			: _tokens(std::move(v)), _offset(0), _instructions({}), _current_pos(0, 0),
			_uninitialized_vars({}), _vars({}), _consts({}), _nextTokenIndex(0) {}
		Analyser(Analyser&&) = delete;
		Analyser(const Analyser&) = delete;
		Analyser& operator=(Analyser) = delete;


        // i2,i3,i4的内容，以大端序（big-endian）写入文件
        typedef int8_t i1;
        typedef int16_t i2;
        typedef int32_t i4;
        typedef int64_t i8;

// u2,u3,u4的内容，以大端序（big-endian）写入文件
        typedef uint8_t u1;
        typedef uint16_t u2;
        typedef uint32_t u4;
        typedef uint64_t u8;
        struct Constant_info {
            // STRING = 0,
            // INT = 1,
            // DOUBLE = 2
            u1 type;
            //union {
            std::string s;
            i4 i;

            //};
            //Constant_info()= delete;
            //Constant_info & operator=(Constant_info&&)= delete;
            //Constant_info(Constant_info&)= delete;
            Constant_info(u1 Type, std::string S)
                    : type(Type), s(std::move(S)) {}

            Constant_info(u1 Type, i4 I)
                    : type(Type), i(I) {}
        };

        //  Constant_info free_func()
        //  {  return {};  }

        std::vector<Constant_info> constv;


        struct Function_info {
            u2 name_index; // name: CO_binary_file.strings[name_index]
            u2 params_size;
            u2 level;
            TokenType type;
            std::map<std::string, int32_t> _uninitialized_vars;
            std::map<std::string, int32_t> _vars;
            std::map<std::string, int32_t> _consts;
            std::vector<Instruction> _instructions;

            int32_t _nextTokenIndex;

            Function_info(int name, u2 lev)
                    : name_index(name), params_size(0), level(lev), _instructions({}), _nextTokenIndex(0) {}
        };

        std::vector<Function_info> funcv;

        struct anaResult{
            std::vector<Constant_info> cons;
            std::vector<Function_info> func;
            std::vector<Instruction> ins;
        }result;


		// 唯一接口
		std::pair<anaResult, std::optional<CompilationError>> Analyse();
	private:
		// 所有的递归子程序

		// <程序>
		std::optional<CompilationError> analyseProgram();
		// <函数声明>
		std::optional<CompilationError> analyseFunctionDefinition();
		// <变量声明>
        std::optional<CompilationError> analyseVariableDeclaration();
        // <常量声明>
        std::optional<CompilationError> analyseConstantDeclaration();
        //<常量初始化>
        std::optional<CompilationError> analyseConstDeclarator();
        //<函数内容>
        std::optional<CompilationError> analyseCompoundStatement();
        // <输出语句>
        std::optional<CompilationError> analysePrintStatement();
        // <输入语句>
        std::optional<CompilationError> analyseScanStatement();
        // <条件语句>
        std::optional<CompilationError> analyseConditionStatement();
        // <循环语句>
        std::optional<CompilationError> analyseLoopStatement();
        // <赋值语句>
        std::optional<CompilationError> analyseAssignmentStatement();
        // <函数调用语句>
        std::optional<CompilationError> analyseFunctionCall();
        // <打印列表语句>
        std::optional<CompilationError>analysePrintableList();
        // <语句序列>
        std::optional<CompilationError> analyseStatementSequence();
        //<语句>
        std::optional<CompilationError>analyseStatement();
        // <表达式>
        std::optional<CompilationError> analyseExpression();
        //<乘除表达式>
        std::optional<CompilationError>analyseMultiplicativeExpression();
        //<单目表达式>
        std::optional<CompilationError>analyseUnaryExpression();
        //<跳转>
        std::optional<CompilationError>analyseJumpStatement();
        //<参数>
        std::optional<CompilationError>analyseParameterDeclaration();


		// <常表达式>
		// 这里的 out 是常表达式的值
		std::optional<CompilationError> analyseConstantExpression(int32_t& out);
		// <项>
		std::optional<CompilationError> analyseItem();
		// <因子>
		std::optional<CompilationError> analyseFactor();

		// Token 缓冲区相关操作

		// 返回下一个 token
		std::optional<Token> nextToken();
		// 回退一个 token
		void unreadToken();

		// 下面是符号表相关操作

		// helper function
		void _add(const Token&, std::map<std::string, int32_t>&);
		// 添加变量、常量、未初始化的变量
		void addVariable(const Token&);
        void addConstant(const Token&);
		void addConstant(const Token&,std::string);
        void addConstant(const Token&,int);
        void addFunction(const Token &tk);
		void addUninitializedVariable(const Token&);

		//是否在同一个作用域内声明过
        bool isLocal(const std::string&);
		// 是否被声明过
		bool isDeclared(const std::string&);
		// 是否是未初始化的变量
		bool isUninitializedVariable(const std::string&);
		// 是否是已初始化的变量
		bool isInitializedVariable(const std::string&);
		// 是否是常量
		bool isConstant(const std::string&);
        bool isFunction(const std::string &);
        // 获得 {变量，常量} 在栈上的偏移
		int32_t getIndex(const std::string&);
        int32_t getFunctionIndex(const std::string&);
	private:
		std::vector<Token> _tokens;
		std::size_t _offset;
        std::vector<Instruction> _instructions;
		std::pair<uint64_t, uint64_t> _current_pos;

		bool nymain= false;



        // 为了简单处理，我们直接把符号表耦合在语法分析里
		// 变量                   示例
		// _uninitialized_vars    int a;
		// _vars                  int a=1;
		// _consts                const a=1;
		std::map<std::string, int32_t> _uninitialized_vars;
		std::map<std::string, int32_t> _vars;
		std::map<std::string, int32_t> _consts;
        std::map<std::string, int32_t> _funcs;
		// 下一个 token 在栈的偏移
		int32_t _nextTokenIndex;
	};
}