//
// Created by Leonard C on 2022/6/24.
//

#ifndef RISC_V_CPU_HPP
#define RISC_V_CPU_HPP

#include "component.hpp"
//#include "Instruction.hpp"

#include <string>
#include <iostream>
using std::string;

class CPU{
public:
    Memory memory;  //总内存
    Registers regs; //总寄存器
    unsigned int pc;
    Decoder decoder;
    ALU alu;

    //模拟的阶段寄存器
    Stage_Register IF_ID, ID_EXE, EXE_MEM, MEM_WB;
    Stage_Register ID, EXE, MEM, WB;

    //---------------------------------------------------------------------------

    CPU() = default;

    static unsigned int hex_to_dec(const string &s) {
        unsigned t = 0;
        for (int i = 0; i < s.length(); ++i) {
            if (s[i] >= '0' && s[i] <= '9') t = (t << 4) + s[i] - '0';
            else t = (t << 4) + s[i] - 'A' + 10;
        }
        return t;
    }

    void input() {
        string s;
        int pos = 0;

        while (getline(std::cin, s)) {
            if (s[0] == '@') { //内存中的数据都是 16 进制数
                pos = hex_to_dec(s.substr(1, 8));
            } else {
                for (int i = 0; i < s.length(); i += 3)
                    memory.mem[pos++] = hex_to_dec(s.substr(i, 2));
            }
        }
    }

    static inline unsigned sign_extend(const unsigned &x, const int &cur_len) {
        unsigned t = -1; // 1111...
        t = (t >> (cur_len - 1) ) << (cur_len - 1); // 3 5
        if (x >> (cur_len - 1)) return x | t;
        else return x;
    }

    void instruction_fetch() {
        pc = EXE_MEM.pc;
        unsigned cmd = memory.read_0(pc, 4);
        IF_ID.ins = Instruction(cmd);
        IF_ID.pc = pc;
//        pc += 4;
    }

    void instruction_decode() {
        //ID 何时赋值？
        ID = IF_ID;
        ID_EXE = ID;
//        ID_EXE.ins = ID.ins;
//        ID_EXE.pc = ID.pc;

        decoder.decode(ID_EXE.ins);

        ID_EXE.op1 = regs.read(ID_EXE.ins.rs1);
        ID_EXE.op2 = regs.read(ID_EXE.ins.rs2);

        if (ID_EXE.ins.name == JAL) {
            ID_EXE.out = ID_EXE.pc + 4;
            alu.cal(ID_EXE.pc, sign_extend(ID_EXE.ins.imm, 21), '+');
            ID_EXE.pc = alu.output;
        }
        if (ID_EXE.ins.name == JALR) {
            ID_EXE.out = ID_EXE.pc + 4;
            alu.cal(ID_EXE.op1, sign_extend(ID_EXE.ins.imm, 12), '+');
            ID_EXE.pc = alu.output & (~1);
        }
        if (ID_EXE.ins.type == 'B') {
            alu.cal(ID_EXE.pc, sign_extend(ID_EXE.ins.imm, 13), '+');
            ID_EXE.out = alu.output;
        }
    }

    void execute() {
        EXE = ID_EXE;
        EXE_MEM = EXE; // ?

        switch (EXE.ins.name) {
            case LUI:
                EXE_MEM.out = EXE.ins.imm;
                break;
            case AUIPC:
                alu.cal(EXE.pc, EXE.ins.imm, '+');
                EXE_MEM.out = alu.output;
                break;
            case ADD:
                alu.cal(EXE.op1, EXE.op2, '+');
                EXE_MEM.out = alu.output;
                break;
            case SUB:
                alu.cal(EXE.op1, EXE.op2, '-');
                EXE_MEM.out = alu.output;
                break;
            case SLL:
                alu.cal(EXE.op1, EXE.op2 & 0x1f, '<');
                break;
            case SLT:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if ((int) EXE.op1 < (int) EXE.op2) EXE_MEM.out = 1;
                else EXE_MEM.out = 0;
                break;
            case SLTU:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 < EXE.op2) EXE_MEM.out = 1;
                else EXE_MEM.out = 0;
                break;
            case XOR:
                alu.cal(EXE.op1, EXE.op2, '^');
                EXE_MEM.out = alu.output;
                break;
            case SRL:
                alu.cal(EXE.op1, EXE.op2 & 0x1f, '>');
                EXE_MEM.out = alu.output;
                break;
            case SRA:
                EXE_MEM.out = EXE.op1 >> (int) EXE.op2;
                break;
            case OR:
                alu.cal(EXE.op1, EXE.op2, '|');
                EXE_MEM.out = alu.output;
                break;
            case AND:
                alu.cal(EXE.op1, EXE.op2, '&');
                EXE_MEM.out = alu.output;
                break;
            case ADDI:
                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '+');
                EXE_MEM.out = alu.output;
                break;
            case SLTI:
//                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '-');
                if ((int) EXE.op1 < (int) sign_extend(EXE.ins.imm, 12)) EXE_MEM.out = 1;
                else EXE_MEM.out = 0;
                break;
            case SLTIU:
//                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '-');
                if (EXE.op1 < sign_extend(EXE.ins.imm, 12)) EXE_MEM.out = 1;
                else EXE_MEM.out = 0;
                break;
            case XORI:
                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '^');
                EXE_MEM.out = alu.output;
                break;
            case ORI:
                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '|');
                EXE_MEM.out = alu.output;
                break;
            case ANDI:
                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '&');
                EXE_MEM.out = alu.output;
                break;
            case SLLI:
                alu.cal(EXE.op1, EXE.ins.rs2, '<');
                EXE_MEM.out = alu.output;
                break;
            case SRLI:
                alu.cal(EXE.op1, EXE.ins.rs2, '>');
                EXE_MEM.out = alu.output;
                break;
            case SRAI:
                EXE_MEM.out = EXE.op1 >> (int) EXE.ins.rs2;
                break;

            case LB:case LH:case LW:case LBU:case LHU: //在 MEM 中修改
                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '+');
                EXE_MEM.out = alu.output;
                break;
            case SB:case SH:case SW: //同理
                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '+');
                EXE_MEM.out = alu.output;
                break;

            case JAL:case JALR: //已经计算完毕
                return;
                break;
            case BEQ:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 == EXE.op2) {
                    EXE_MEM.pc = EXE.out;
                    return;
                }
                break;
            case BNE:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 != EXE.op2) {
                    EXE_MEM.pc = EXE.out;
                    return;
                }
                break;
            case BLT:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if ((int) EXE.op1 < (int) EXE.op2) {
                    EXE_MEM.pc = EXE.out;
                    return;
                }
                break;
            case BGE:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if ((int) EXE.op1 >= (int) EXE.op2) {
                    EXE_MEM.pc = EXE.out;
                    return;
                }
                break;
            case BLTU:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 < EXE.op2) {
                    EXE_MEM.pc = EXE.out;
                    return;
                }
                break;
            case BGEU:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 >= EXE.op2) {
                    EXE_MEM.pc = EXE.out;
                    return;
                }
                break;

            case END:
                break;
        }

        EXE_MEM.pc = EXE.pc + 4;
    }

    void memory_access() {
        MEM = EXE_MEM;
        MEM_WB = MEM;

        switch (MEM.ins.name) {
            case LB: //符号位扩展
                MEM_WB.out = memory.read(MEM.out, 1);
                break;
            case LH:
                MEM_WB.out = memory.read(MEM.out, 2);
                break;
            case LW:
                MEM_WB.out = memory.read(MEM.out, 4);
                break;
            case LBU: // 0扩展
                MEM_WB.out = memory.read_0(MEM.out, 1);
                break;
            case LHU:
                MEM_WB.out = memory.read_0(MEM.out, 2);
                break;
            case SB:
                memory.write(MEM.out, MEM.op2, 1);
                break;
            case SH:
                memory.write(MEM.out, MEM.op2, 2);
                break;
            case SW:
                memory.write(MEM.out, MEM.op2, 4);
                break;
        }
    }

    void write_back_to_register() {
        WB = MEM_WB;
        //要判断，S类指令不需要 write_back
        if (WB.ins.type != 'S' && WB.ins.type != 'B')
            regs.write(WB.ins.rd, WB.out);
    }

    void print() {
//        for (int i = 1; i < 32; ++i)
//            printf("%u ", regs.reg[i]);
//        std::cout << std::endl;
        for (int i = 1;i < 5000; ++i)
            printf("%u ", memory.mem[i]);
        std::cout << std::endl;
    }

    void run() {
        int clk = 0;
        while (1) {
            ++clk;
            std::cout << clk << ": " << WB.pc << " : ";
            instruction_fetch();
            instruction_decode();

            if (ID_EXE.ins.name == END) break;

            execute();
            memory_access();
            write_back_to_register();

            std::cout << WB.pc << " - ";
            print();
            printf("%x\n", WB.ins.cmd);

            regs.reg[0] = 0; //每次操作后都要手动清空
        }
        printf("%u\n", regs.reg[10] & 255u);
    }
};

#endif //RISC_V_CPU_HPP
