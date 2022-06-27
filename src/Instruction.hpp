//
// Created by Leonard C on 2022/6/24.
//

#ifndef CPU_HPP_INSTRUCTION_HPP
#define CPU_HPP_INSTRUCTION_HPP

enum TYPE{
    LUI,AUIPC,  //U类型 用于操作长立即数的指令 0~1
    ADD,SUB,SLL,SLT,SLTU,XOR,SRL,SRA,OR,AND,  //R类型 寄存器间操作指令 2~11
    JALR,LB,LH,LW,LBU,LHU,ADDI,SLTI,SLTIU,XORI,ORI,ANDI,SLLI,SRLI,SRAI,  //I类型 短立即数和访存Load操作指令 12~26
    SB,SH,SW,  //S类型 访存Store操作指令 27~29
    JAL,  //J类型 用于无条件跳转的指令 30
    BEQ,BNE,BLT,BGE,BLTU,BGEU, //B类型 用于有条件跳转的指令 31~36
    END
};

enum Stages{IF, ID, EXE, MEM, WB};

class Instruction{
public:
    unsigned int rd, rs1, rs2, imm, opcode, func3, func7;
    unsigned int cmd;
    TYPE name;
    char type;

    Instruction() = default;
    explicit Instruction(const unsigned &_cmd) : cmd(_cmd) {
        opcode = cmd & 0x7f; //取出后 7 位
        func3 = (cmd >> 12) & 0x7; //
        func7 = (cmd >> 25) & 0x7f; //前7位
        rd = (cmd >> 7) & 0x1f;
        rs1 = (cmd >> 15) & 0x1f;
        rs2 = (cmd >> 20) & 0x1f;
    }
};

class Decoder{
public:
    void decode(Instruction &ins) {
        if (ins.cmd == (int) 0x0ff00513) {
            ins.name = END;
            return;
        }

        if (ins.opcode == 0x37 || ins.opcode == 0x17) {
            ins.type = 'U';
            ins.imm = (ins.cmd >> 12) << 12;
            if (ins.opcode == 0x37) ins.name = LUI;
            else ins.name = AUIPC;

        } else if (ins.opcode == 0x33) {
            ins.type = 'R';
            ins.imm = 0;
            if (ins.func3 == 0x0 && ins.func7 == 0x00) ins.name = ADD;
            else if (ins.func3 == 0x0 && ins.func7 == 0x20) ins.name = SUB;
            else if (ins.func3 == 0x1) ins.name = SLL;
            else if (ins.func3 == 0x2) ins.name = SLT;
            else if (ins.func3 == 0x3) ins.name = SLTU;
            else if (ins.func3 == 0x4) ins.name = XOR;
            else if (ins.func3 == 0x5 && ins.func7 == 0x00) ins.name = SRL;
            else if (ins.func3 == 0x5 && ins.func7 == 0x20) ins.name = SRA;
            else if (ins.func3 == 0x6) ins.name = OR;
            else if (ins.func3 == 0x7) ins.name = AND;

        } else if (ins.opcode == 0x67 || ins.opcode == 0x3 || ins.opcode == 0x13) {
            ins.type = 'I';
            ins.imm = ins.cmd >> 20;
            if (ins.opcode == 0x67) ins.name = JALR;
            else if (ins.opcode == 0x3) {
                if (ins.func3 == 0x0) ins.name = LB;
                else if (ins.func3 == 0x1) ins.name = LH;
                else if (ins.func3 == 0x2) ins.name = LW;
                else if (ins.func3 == 0x4) ins.name = LBU;
                else if (ins.func3 == 0x5) ins.name = LHU;
            } else {
                if (ins.func3 == 0x0) ins.name = ADDI;
                else if (ins.func3 == 0x2) ins.name = SLTI;
                else if (ins.func3 == 0x3) ins.name = SLTIU;
                else if (ins.func3 == 0x4) ins.name = XORI;
                else if (ins.func3 == 0x6) ins.name = ORI;
                else if (ins.func3 == 0x7) ins.name = ANDI;
                else if (ins.func3 == 0x1) ins.name = SLLI;
                else if (ins.func3 == 0x5 && ins.func7 == 0x00) ins.name = SRLI;
                else if (ins.func3 == 0x5 && ins.func7 == 0x20) ins.name = SRAI;
            }

        } else if (ins.opcode == 0x23) {
            ins.type = 'S';
            ins.imm = ((ins.cmd >> 25) << 5) | ((ins.cmd >> 7) & 0x1f);
            if (ins.func3 == 0x0) ins.name = SB;
            else if (ins.func3 == 0x1) ins.name = SH;
            else if (ins.func3 == 0x2) ins.name = SW;

        } else if (ins.opcode == 0x6f) {
            ins.type = 'J';
            ins.imm = ( ((ins.cmd >> 12) & 0xff) << 12) | ( ((ins.cmd >> 20) & 0x1) << 11) |
                      ( ((ins.cmd >> 21) & 0x3ff) << 1) | ( ((ins.cmd >> 31) & 1) << 20);
            ins.name = JAL;

        } else if (ins.opcode == 0x63) {
            ins.type = 'B';
            ins.imm = ( ((ins.cmd >> 7) & 0x1) << 11) |
                      ( ((ins.cmd >> 8) & 0xf) << 1) |
                      ( ((ins.cmd >> 25) & 0x3f) << 5) |
                      ( ((ins.cmd >> 31) & 0x1) << 12);
            if (ins.func3 == 0x0) ins.name = BEQ;
            else if (ins.func3 == 0x1) ins.name = BNE;
            else if (ins.func3 == 0x4) ins.name = BLT;
            else if (ins.func3 == 0x5) ins.name = BGE;
            else if (ins.func3 == 0x6) ins.name = BLTU;
            else if (ins.func3 == 0x7) ins.name = BGEU;
        }
    }

};

#endif //CPU_HPP_INSTRUCTION_HPP
