//
// Created by Leonard C on 2022/6/24.
//

#ifndef CPU_HPP_COMPONENT_HPP
#define CPU_HPP_COMPONENT_HPP

#include "Instruction.hpp"

#include <cstdio>
#include <cstring>

class Clock{
public:
    int stall[5];
    size_t tick;

    Clock() = default;

    void timing() {
        tick++;
        
    }

};

class Memory{
public:
    unsigned char mem[500000];

    Memory() {
        memset(mem, 0, sizeof(mem));
    }

    unsigned read(size_t pos, int len) const { //符号位扩展
        unsigned ret;
        switch (len) {
            case 1:
                ret = (char) mem[pos];
                break;
            case 2:
                ret = (short) ((unsigned short) mem[pos] + (unsigned short) (mem[pos + 1] << 8) );
                break;
            case 4:
                ret = (int) ((unsigned int) mem[pos] + (unsigned int) (mem[pos + 1] << 8) +
                             (unsigned int) (mem[pos + 2] << 16) + (unsigned int) (mem[pos + 3] << 24));
                break;
        }
        return ret;
    }

    unsigned read_0(size_t pos, int len) const { //0 扩展
        unsigned ret;
        switch (len) {
            case 1:
                ret = mem[pos];
                break;
            case 2:
                ret = (unsigned short) mem[pos] + (unsigned short) (mem[pos + 1] << 8);
                break;
            case 4:
                ret = (unsigned int) mem[pos] + (unsigned int) (mem[pos + 1] << 8) +
                      (unsigned int) (mem[pos + 2] << 16) + (unsigned int) (mem[pos + 3] << 24);
                break;
        }
        return ret;
    }

    void write(size_t pos, const unsigned &_data, int len) {
        switch (len) {
            case 1:
                mem[pos] = _data;
                break;
            case 2:
                mem[pos] = _data;
                mem[pos + 1] = (_data & 0xff00) >> 8;
                break;
            case 4:
                mem[pos] = _data;
                mem[pos + 1] = (_data & 0xff00) >> 8;
                mem[pos + 2] = (_data & 0xff0000) >> 16;
                mem[pos + 3] = (_data & 0xff000000) >> 24;
        }
    }
};

class Registers{
public:
    unsigned int reg[32];

    Registers() {
        memset(reg, 0, sizeof(reg));
    }

    void write(size_t pos, const unsigned &_data) {
        reg[pos] = _data;
    }

    unsigned read(size_t pos) const {
        return reg[pos];
    }
};

class Stage_Register{
public:
    Instruction ins;
    unsigned int pc, out, op1, op2;

    Stage_Register() = default;
};

class ALU{
public:
    unsigned int output;

    ALU() = default;

    void cal(unsigned a, unsigned b, const char &opt) {
        switch (opt) {
            case '+':
                output = a + b;
                break;
            case '-':
                output = a - b;
                break;
            case '>': //位运算右移
                output = a >> b;
                break;
            case '<': //左移
                output = a << b;
                break;
            case '&':
                output = a & b;
                break;
            case '|':
                output = a | b;
                break;
            case '^':
                output = a ^ b;
                break;
        }

    }
};

#endif //CPU_HPP_COMPONENT_HPP
