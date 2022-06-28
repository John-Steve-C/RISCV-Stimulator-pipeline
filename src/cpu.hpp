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

    Clock clk;
    BUS bus; //信号总线

    //---------------------------------------------------------------------------

    CPU() = default;

    static unsigned int hex_to_dec(const string &s) {
        unsigned t = 0;
        for (char i : s) {
            if (i >= '0' && i <= '9') t = (t << 4) + i - '0';
            else t = (t << 4) + i - 'A' + 10;
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
        if (clk.stall_check(IF_stage)) return;

//        pc = EXE_MEM.pc;
        // 如果没有 jump 操作，那么 pc += 4
        // 否则应该跳转的位置已经保存在 bus 中
        unsigned cmd = memory.read_0(pc, 4);
        IF_ID.ins = Instruction(cmd);
        IF_ID.pc = pc; //表示该指令读入的位置
        pc += 4;
    }

    void instruction_decode() {
        if (clk.stall_check(ID_stage)) return;
        //ID 何时赋值？ 在 pass_cycle 中实现
//        ID = IF_ID;
        ID_EXE = ID;

        decoder.decode(ID_EXE.ins);

        ID_EXE.op1 = regs.read(ID_EXE.ins.rs1);
        ID_EXE.op2 = regs.read(ID_EXE.ins.rs2);

        if (ID_EXE.ins.name == JAL) {
            ID_EXE.out = ID_EXE.pc + 4;
            alu.cal(ID_EXE.pc, sign_extend(ID_EXE.ins.imm, 21), '+');
//            ID_EXE.pc = alu.output;
            bus.jump = true;
            bus.target_pc = alu.output;
        }
        if (ID_EXE.ins.name == JALR) {
            ID_EXE.out = ID_EXE.pc + 4;
            alu.cal(ID_EXE.op1, sign_extend(ID_EXE.ins.imm, 12), '+');
//            ID_EXE.pc = alu.output & (~1);
            bus.jump = true;
            bus.target_pc = alu.output & (~1);
        }
        if (ID_EXE.ins.type == 'B') {
            alu.cal(ID_EXE.pc, sign_extend(ID_EXE.ins.imm, 13), '+');
//            ID_EXE.out = alu.output;
            bus.branch = true;
            bus.target_pc = alu.output;
        }
    }

    void execute() {
        if (clk.stall_check(EXE_stage)) return;
//        EXE = ID_EXE;
        EXE_MEM = EXE;

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
                EXE_MEM.out = alu.output; // 暂时表示 mem 的 pos
                bus.memory_access = true;
                break;
            case SB:case SH:case SW: //同理
                alu.cal(EXE.op1, sign_extend(EXE.ins.imm, 12), '+');
                EXE_MEM.out = alu.output;
                bus.memory_access = true;
                break;

            case JAL:case JALR: //已经计算完毕
                return;
                break;
            case BEQ:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 == EXE.op2) {
                    EXE_MEM.pc = bus.target_pc; // 这里的 pc 表示的是执行后，正确跳转的位置
                    bus.jump = true;
                    return;
                }
                break;
            case BNE:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 != EXE.op2) {
                    EXE_MEM.pc = bus.target_pc;
                    bus.jump = true;
                    return;
                }
                break;
            case BLT:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if ((int) EXE.op1 < (int) EXE.op2) {
                    EXE_MEM.pc = bus.target_pc;
                    bus.jump = true;
                    return;
                }
                break;
            case BGE:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if ((int) EXE.op1 >= (int) EXE.op2) {
                    EXE_MEM.pc = bus.target_pc;
                    bus.jump = true;
                    return;
                }
                break;
            case BLTU:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 < EXE.op2) {
                    EXE_MEM.pc = bus.target_pc;
                    bus.jump = true;
                    return;
                }
                break;
            case BGEU:
//                alu.cal(EXE.op1, EXE.op2, '-');
                if (EXE.op1 >= EXE.op2) {
                    EXE_MEM.pc = bus.target_pc;
                    bus.jump = true;
                    return;
                }
                break;

            case END:
                break;
        }

        EXE_MEM.pc = EXE.pc + 4; // 这里的 pc 表示的是执行后，正确跳转的位置
    }

    void memory_access() {
        if (clk.stall_check(MEM_stage)) return;
//        MEM = EXE_MEM;
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
        if (clk.stall_check(WB_stage)) return;
//        WB = MEM_WB;
        //要判断，S类/B类 指令不需要 write_back
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

    void pass_cycle() {
        if (!clk.not_update_check(ID_stage)) {
            ID = IF_ID;
            if (!clk.stall_check(ID_stage)) IF_ID.clear();
        }
        if (!clk.not_update_check(IF_stage)) {
            EXE = ID_EXE;
            if (!clk.stall_check(EXE_stage)) ID_EXE.clear();
        }
        if (!clk.not_update_check(EXE_stage)) {
            MEM = EXE_MEM;
            if (!clk.stall_check(MEM_stage)) EXE_MEM.clear();
        }
        if (!clk.not_update_check(WB_stage)) {
            WB = MEM_WB;
            if (!clk.stall_check(WB_stage)) MEM_WB.clear();
        }
    }

    void judge() { //进行是否需要停顿的判断
        if (bus.memory_access) { //用三个周期执行
            bus.memory_access = false;
            clk.mem_tick = 2;
            clk.stall_stage(MEM_stage, 2);
            clk.stall_stage(ID_stage, 2);
            clk.stall_stage(IF_stage, 2);
            clk.stall_stage(EXE_stage, 2);
        }
        if (bus.jump) {
            bus.jump = false;
            pc = bus.target_pc;
            IF_ID.clear();
        }
    }

    void run() {
        while (1) {
//            std::cout << clk << ": " << WB.pc << " : ";
            if (EXE.ins.name == END) break;

            pass_cycle();

            //倒序执行，实现伪并行
            write_back_to_register();
            memory_access();
            execute();
            instruction_decode();
            instruction_fetch();

            clk.timing();

            judge();

//            std::cout << WB.pc << " - ";
//            print();
//            printf("%x\n", WB.ins.cmd);

            regs.reg[0] = 0; //每次操作后都要手动清空
        }
        printf("%u\n", regs.reg[10] & 255u);
    }
};

#endif //RISC_V_CPU_HPP
