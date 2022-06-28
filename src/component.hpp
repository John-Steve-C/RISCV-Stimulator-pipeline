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
    int stall[5], cycle, not_update[5], mem_tick;
    // cycle 表示当前执行到的时间
    // mem_tick 表示 距离内存访问结束，需要的时间

    Clock() = default;

    void timing() {
        cycle++;
        for (int i = 0;i < 5; ++i) {
            if (stall[i]) --stall[i];
            if (!mem_tick && not_update[i]) not_update[i]--;
        }
        if (mem_tick > 0) mem_tick--;
    }

    void stall_stage(Stages stage, int time) {
        stall[stage] += time;
    }

    void not_update_stage(Stages stage, int time) {
        not_update[stage] += time;
    }

    bool stall_check(Stages stage) { //判断当前阶段是否被推迟
        return stall[stage];
    }

    bool not_update_check(Stages stage) {
        return not_update[stage];
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
    //pc 表示当前阶段对应的命令，是从 哪一个位置读入的
    //out 表示输出到 reg[rd] 的值

    Stage_Register() = default;

    void clear() {
        pc = out = op1 = op2 = 0;
        ins.clear();
    }
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

class BUS{
public:
    bool memory_access, jump, branch;
    unsigned target_pc;

    BUS() = default;
    void clear() {
        memory_access = jump = branch = false;
        target_pc = 0;
    }
};

#endif //CPU_HPP_COMPONENT_HPP
