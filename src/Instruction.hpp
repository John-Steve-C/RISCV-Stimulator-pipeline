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

class Instruction{
public:
    unsigned int rd, rs1, rs2, imm, opcode, func3, func7;
    unsigned int cmd;
    TYPE name;
    char type;

    Instruction() = default;
    Instruction(const unsigned &_cmd) : cmd(_cmd) {
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

    void decode(Instruction &inc) {
        if (inc.opcode == (int) 0x0ff00513) {
            inc.name = END;
            return;
        }

        if (inc.opcode == 0x37 || inc.opcode == 0x17) {
            inc.type = 'U';
//            inc.imm = (inc.cmd >> 12) << 12;
            if (inc.opcode == 0x37) inc.name = LUI;
            else inc.name = AUIPC;

        } else if (inc.opcode == 0x33) {
            inc.type = 'R';
            if (inc.func3 == 0x0 && inc.func7 == 0x00) inc.name = ADD;
            else if (inc.func3 == 0x0 && inc.func7 == 0x20) inc.name = SUB;
            else if (inc.func3 == 0x1) inc.name == SLL;
            else if (inc.func3 == 0x2) inc.name == SLT;
            else if (inc.func3 == 0x3) inc.name = SLTU;
            else if (inc.func3 == 0x4) inc.name = XOR;
            else if (inc.func3 == 0x5 && inc.func7 == 0x00) inc.name = SRL;
            else if (inc.func3 == 0x5 && inc.func7 == 0x20) inc.name = SRA;
            else if (inc.func3 == 0x6) inc.name = OR;
            else if (inc.func3 == 0x7) inc.name = AND;

        } else if (inc.opcode == 0x67 || inc.opcode == 0x03 || inc.opcode == 0x13) {
            inc.type = 'I';
            if (inc.opcode == 0x67) inc.name = JALR;
            else if (inc.opcode == 0x03) {
                if (inc.func3 == 0x0) inc.name = LB;
                else if (inc.func3 == 0x1) inc.name = LH;
                else if (inc.func3 == 0x2) inc.name = LW;
                else if (inc.func3 == 0x4) inc.name = LBU;
                else if (inc.func3 == 0x5) inc.name = LHU;
            } else {
                if (inc.func3 == 0x0) inc.name = ADDI;
                else if (inc.func3 == 0x2) inc.name = SLTI;
                else if (inc.func3 == 0x3) inc.name = SLTIU;
                else if (inc.func3 == 0x4) inc.name = XORI;
                else if (inc.func3 == 0x6) inc.name = ORI;
                else if (inc.func3 == 0x7) inc.name = ANDI;
                else if (inc.func3 == 0x1) inc.name = SLLI;
                else if (inc.func3 == 0x5 && inc.func7 == 0x00) inc.name = SRLI;
                else if (inc.func3 == 0x5 && inc.func7 == 0x20) inc.name = SRAI;
            }

        } else if (inc.opcode == 0x23) {
            inc.type = 'S';
            if (inc.func3 == 0x0) inc.name = SB;
            else if (inc.func3 == 0x1) inc.name = SH;
            else if (inc.func3 == 0x2) inc.name = SW;

        } else if (inc.opcode == 0x6f) {
            inc.type = 'J';
            inc.name = JAL;

        } else if (inc.opcode == 0x63) {
            inc.type = 'B';
            if (inc.func3 == 0x0) inc.name = BEQ;
            else if (inc.func3 == 0x1) inc.name = BNE;
            else if (inc.func3 == 0x4) inc.name = BLT;
            else if (inc.func3 == 0x5) inc.name = BGE;
            else if (inc.func3 == 0x6) inc.name = BLTU;
            else if (inc.func3 == 0x7) inc.name = BGEU;
        }
    }

};

#endif //CPU_HPP_INSTRUCTION_HPP
