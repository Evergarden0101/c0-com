#pragma once

#include <cstdint>
#include <utility>

namespace C0 {

    enum Operation {
        snew = 0,
        ipush,
        bipush,
        loada,
        iload,
        istore,
        iprint,
        cprint,
        iadd,
        isub,
        imul,
        idiv,
        iret,
        ret,
        iscan,
        ineg,
        loadc,
        icmp,
        jge,
        je,
        jne,
        jl,
        jg,
        jle,
        jmp,
        call,
    };

    class Instruction final {
    private:
        using int32_t = std::int32_t;
    public:
        //friend void swap(Instruction& lhs, Instruction& rhs);
    public:
        Instruction(Operation opr) : _opr(opr), _x(0), _y(0) {}

        Instruction(Operation opr, int32_t x) : _opr(opr), _x(x), _y(0) {}

        Instruction(Operation opr, int32_t x, int32_t y) : _opr(opr), _x(x), _y(y) {}

        //Instruction() : Instruction(Operation::ILL, 0){}
        Instruction(const Instruction &i) {
            _opr = i._opr;
            _x = i._x;
            _y = i._y;
        }
        //Instruction(Instruction&& i) :Instruction() { swap(*this, i); }
        //Instruction& operator=(Instruction i) { swap(*this, i); return *this; }
        //bool operator==(const Instruction& i) const { return _opr == i._opr && _x == i._x; }

        Operation GetOperation() const { return _opr; }

        int32_t GetX() const { return _x; }

        int32_t GetY() const { return _y; }

        void SetX(int32_t x) { _x = x; }

    private:
        Operation _opr;
        int32_t _x;
        int32_t _y;
    };
/*
	inline void swap(Instruction& lhs, Instruction& rhs) {
		using std::swap;
		swap(lhs._opr, rhs._opr);
		swap(lhs._x, rhs._x);
	}
*/

}