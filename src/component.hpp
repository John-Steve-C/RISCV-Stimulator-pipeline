//
// Created by Leonard C on 2022/6/24.
//

#ifndef CPU_HPP_COMPONENT_HPP
#define CPU_HPP_COMPONENT_HPP

class Clock{

};

class Memory{
public:
    unsigned char mem[500000];

    Memory() {
        memset(mem, 0, sizeof(mem));
    }
};

class Registers{
public:
    unsigned int reg[32];

    Registers() {
        memset(reg, 0, sizeof(reg));
    }

    void write(size_t pos, const unsigned &_data) {
        if (pos) reg[pos] = _data;
    }

    unsigned read(size_t pos) const {
        return reg[pos];
    }
};

class ALU{
public:
    unsigned int output;

    ALU() = default;

};

#endif //CPU_HPP_COMPONENT_HPP
