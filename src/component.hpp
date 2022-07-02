//
// Created by Leonard C on 2022/6/24.
//

#ifndef CPU_HPP_COMPONENT_HPP
#define CPU_HPP_COMPONENT_HPP

#include "Instruction.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>

class Clock{
public:
    int stall[5], cycle, not_update[5], mem_tick;
    // cycle 表示当前执行到的周期
    // mem_tick 表示 距离内存访问结束，需要的时间
    // stall[i] 表示 i 阶段需要停顿的时间
    // not_update[i] 则表示 i 阶段的数据，直到更新需要的时间

    Clock() = default;

    void timing() {
        cycle++;
        for (int i = 0;i < 5; ++i) {
            if (stall[i]) --stall[i];
            if (!mem_tick && not_update[i]) not_update[i]--;
            //还需要内存访问时，其他操作一律停顿
        }
        if (mem_tick > 0) mem_tick--;
    }

    void stall_stage(Stages stage, int time) {
        stall[stage] += time;
    }

    void not_update_stage(Stages stage, int time) {
        not_update[stage] += time;
    }

    void display() {
        for (int i = 0;i < 5; ++i) std::cout << stall[i] << ' ';
        std::cout << " // ";
        for (int i = 0;i < 5; ++i) std::cout << not_update[i] << ' ';
        std::cout << std::endl;
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

class Stage_Register{ //临时寄存器
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

class ALU{ //计算单元
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

class BUS{ // 信号总线，传递特殊信号
public:
    bool memory_access, jump, branch, delay, predict_ans;
    unsigned target_pc;

    BUS() = default;
    void clear() {
        jump = branch = predict_ans = false;
    }
};

//-----------------------------以下不是硬件部分-----------------------

class Branch_Predictor{ //用于分支预测
public:
    unsigned int ans[4][2], two_counter[1 << 12]; // 2位饱和计数器预测
    int wrong, correct, now_pc;

    Branch_Predictor() : wrong(0), correct(0), now_pc(0) {
        // 0 -> 00, 1 -> 01, 2 -> 10, 3 -> 11
        // strong(not jump) -> weak(not) -> weak(jump) -> strong(jump)
        // strong: 如果预测正确就不动，否则变成弱
        // weak: 如果正确，变成强，否则变成另一类的若
        ans[0][0] = 0, ans[0][1] = 1;
        ans[1][0] = 0, ans[1][1] = 2;
        ans[2][0] = 1, ans[2][1] = 3;
        ans[3][0] = 2, ans[3][1] = 3;

        memset(two_counter, 0, sizeof(two_counter));
    }

    bool predict(unsigned _pc = 0) {
        now_pc = _pc & 0xfff; //取后12位
        return two_counter[now_pc] > 1; // 10, 11: jump, success
    }

    void update(bool _jump = false) { // 用预测结果更新 此命令的跳转情况
        two_counter[now_pc] = ans[ two_counter[now_pc] ][_jump];
        now_pc = 0;
    }
};

class Data_Transmitter{ //用于 data_forwarding
public:
    unsigned int EXE_rd[2], EXE_val[2], MEM_rd[2], MEM_val[2];
    //二维数组，[0] 表示旧数据，[1] new

    Data_Transmitter() = default;

    void get_data(unsigned rs, unsigned &ret) {
        if (rs == EXE_rd[0]) ret = EXE_val[0];
        if (rs == MEM_rd[0]) ret = MEM_val[0];
    }

    void save(unsigned rd, unsigned val, Stages stage) {
        if (!rd) return;
        if (stage == EXE_stage) {
            EXE_rd[1] = rd;
            EXE_val[1] = val;
        } else if (stage == MEM_stage){
            MEM_rd[1] = rd;
            MEM_val[1] = val;
        }
    }

    void update(bool EXE_stall, bool MEM_stall) { // 旧 <- 新
        if (!EXE_stall) {
            EXE_rd[0] = EXE_rd[1];
            EXE_val[0] = EXE_val[1];
            EXE_rd[1] = EXE_val[1] = 0;
        }
        if (!MEM_stall) {
            MEM_rd[0] = MEM_rd[1];
            MEM_val[0] = MEM_val[1];
            MEM_rd[1] = MEM_val[1] = 0;
        }
    }

    void display() {
        std::cout << EXE_rd[0] << ' ' << EXE_val[0] << ' ' << MEM_rd[0] << ' ' << MEM_val[0] << '\n';
        std::cout << EXE_rd[1] << ' ' << EXE_val[1] << ' ' << MEM_rd[1] << ' ' << MEM_val[1] << '\n';
    }
};

#endif //CPU_HPP_COMPONENT_HPP
