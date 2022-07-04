## RISC_V Stimulator

### 简介

RISC-V（发音为“risk-five”）是一个基于精简指令集（RISC）原则的开源指令集架构（ISA），简易解释为开源软体运动相对应的一种“开源硬体”。该项目2010年始于加州大学柏克莱分校，但许多贡献者是该大学以外的志愿者和行业工作者。

本次作业里涉及的指令是 RV32I 的一部分。[32位基础整数指令集](https://www.cnblogs.com/mikewolf2002/p/11196680.html) ，它支持32位寻址空间，支持字节地址访问，寄存器也是32位整数寄存器。

### 实现思路

![流程图](https://s3.bmp.ovh/imgs/2022/07/04/8b38a4c5ece04bdd.jpg)

采用的是 **五级流水**（pipeline）

1. 实现 [一级流水](https://acm.sjtu.edu.cn/OnlineJudge/code?submit_id=181614) ，即顺序执行
2. 实现 五级流水，把整个流程 划分为五个阶段：
    - Instruction Fetch (IF)：从寄存器中读取指令
    - Instruction Decode (ID)：解码指令，破译出指令类型、操作数、操作地址等等
    - Execute (EXE)：执行指令，计算出结果
    - Memory Access (MEM)：把对应信息写入内存中（对于 load、save 类指令）（要求用三个周期实现）
    - Write Back to rd register (WB)：把结果写入对应的寄存器中（大部分命令）

   由于空间上的互异，不同指令的不同阶段可以 **并行**。
3. 把五级流水修改为 **逆序执行**（实现伪并行），那么此时就会遇到三种 [hazard](https://blog.csdn.net/qq_45677520/article/details/122806845)
    - structural hazard（某个寄存器需要同时读写数据）（本次作业不出现）
    - data hazard（计算结果尚未写入寄存器/内存，其他命令就要从中读取）
    - control hazard（分支跳转，如果要跳转，那么逆序读入的数据就要清空）
4. 事实上，可以支持 `random_run`，即任意修改 `run` 函数中五个阶段的执行顺序，不影响结果的正确性。

### hazard 的解决方法

最简单的办法：停顿，等到上一条指令执行完毕，再进行下一条指令（但效率显然会很低）

#### data hazard：data_forwarding
       
注意到，主要是在 EXE_MEM 和 MEM_WB 阶段，会发生数据　**已经计算出，但来不及写回**　的情况。

> 这里的已经计算，指的是 数据计算的 cycle（EXE/MEM） 发生在 ID 的 cycle **之前**。
   
因此，定义 `Data_Transmitter` 类，用来记录这两个阶段的旧数据和新数据，每个 cycle 进行更新。
   
在 ID 阶段读取出旧数据，计算出结果时保存新数据。每个 cycle 的五个流水执行后，如果不需要 stall，就用新数据来更新旧数据。
   
如果 hazard 在 **同一个 cycle 中** 发生，那么只能采取停顿：
- MEM_WB 阶段，命令为访问内存（LB、LH...）
- EXE_MEM 阶段，非 load 类指令

#### control hazard：分支预测

用 `two_counter[1 << 12]` 来保存每条指令（取后12位，视为 hash 操作）的预测结果。

采用二位饱和预测，`ans[4][2]` 数组表示预测结果。

`0 -> 00, 1 -> 01, 2 -> 10, 3 -> 11`。对应：强不跳、弱不跳、弱跳、强跳
  
- strong: 如果预测正确就不动，否则变成同类的弱
- weak: 如果正确，变成强，否则变成另一类的弱

具体执行时，

1. 在 ID 阶段，每当读取到跳转指令时，就更新 bus 中的 jump/branch 标记
2. 在 EXE 阶段，可以得到正确的跳转结果。如果与先前的预测不符，就把 pc 指针回退到正确的位置，同时清空 阶段寄存器。
3. 五个流水执行后，如果有分支命令，就进行预测。如果预测结果为跳转，就清空 IF_ID

### 具体架构

#### `Instruction.hpp`

包括了指令类 `Instruction`，以及公用的解码器 `class Decoder`

```cpp
enum TYPE{
    NOP, // 空指令 0
    LUI,AUIPC,  //U类型 用于操作长立即数的指令 1~2
    ADD,SUB,SLL,SLT,SLTU,XOR,SRL,SRA,OR,AND,  //R类型 寄存器间操作指令 3~12
    JALR,LB,LH,LW,LBU,LHU,ADDI,SLTI,SLTIU,XORI,ORI,ANDI,SLLI,SRLI,SRAI,  //I类型 短立即数和访存Load操作指令 13~27
    SB,SH,SW,  //S类型 访存Store操作指令 28~30
    JAL,  //J类型 用于无条件跳转的指令 31
    BEQ,BNE,BLT,BGE,BLTU,BGEU, //B类型 用于有条件跳转的指令 32~37
    END //特殊的结束指令 38
};
enum Stages{IF_stage, ID_stage, EXE_stage, MEM_stage, WB_stage};

class Instruction{
public:
    unsigned int rd, rs1, rs2, imm, opcode, func3, func7;
    unsigned int cmd;
    TYPE name;
    char type;

    Instruction() = default;
    explicit Instruction(const unsigned &_cmd) : cmd(_cmd) {}
    void clear() {}
};

class Decoder{
public:
    void decode(Instruction &ins) {}
};
```

#### `component.hpp`

模拟实现 CPU 内部的组成，比如时钟、寄存器、内存、算数逻辑单元ALU

```cpp
class Clock{
public:
    int stall[5], cycle, not_update[5], mem_tick;
    // cycle 表示当前执行到的周期
    // mem_tick 表示 距离内存访问结束，需要的时间
    // stall[i] 表示 i 阶段需要停顿的时间
    // not_update[i] 则表示 i 阶段的数据，直到更新需要的时间

    Clock() = default;
    void timing() {} //计时器更新，
    void stall_stage(Stages stage, int time) {}
    void not_update_stage(Stages stage, int time) {}
    void display() {}
};

class Memory{
public:
    unsigned char mem[500000]; // 一个 char --> 1 byte --> 8 bit （8位二进制）
    // 一个内存单元，正好是 8位 

    Memory() {}
    unsigned read(size_t pos, int len) const {} //符号位扩展
    unsigned read_0(size_t pos, int len) const {} //0 扩展
    void write(size_t pos, const unsigned &_data, int len) {}
};

class Registers{
public:
    unsigned int reg[32]; // int --> 4 byte --> 32 bit

    Registers() {}
    void write(size_t pos, const unsigned &_data) {}
    unsigned read(size_t pos) const {}
};

class Stage_Register{ //临时寄存器
public:
    Instruction ins;
    unsigned int pc, out, op1, op2;
    //pc 表示当前阶段对应的命令，是从 哪一个位置读入的
    //out 表示输出到 reg[rd] 的值

    Stage_Register() = default;
    void clear() {}
};

class ALU{ //计算单元
public:
    unsigned int output; // 计算结果

    ALU() = default;
    void cal(unsigned a, unsigned b, const char &opt) {}
};

class BUS{ // 信号总线，传递特殊信号
public:
    bool memory_access, jump, branch, delay, predict_ans; 
    // 是否发生内存访问/是否需要跳转/是否为分支跳转语句/是否需要延迟跳转/分支预测的结果
    unsigned target_pc; // 如果需要跳转，pc指针的目标位置

    BUS() = default;
    void clear() {}
};

class Branch_Predictor{ //用于分支预测
public:
    unsigned int ans[4][2], two_counter[1 << 12]; // ans[]: 2位饱和计数器预测
    // two_counter[i]: 对于 pc后12位 = i 的指令的分支预测结果
    // 为什么取 命令的后12位？因为后7-12位是类别标识符，后5位是 rd 
    // 实际上相当于 一次哈希
    
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

    bool predict(unsigned _pc = 0) {}
    void update(bool _jump = false) {} // 用预测结果更新 计数器
};

class Data_Transmitter{ //用于 data_forwarding
public:
    unsigned int EXE_rd[2], EXE_val[2], MEM_rd[2], MEM_val[2];
    //二维数组，[0] 表示旧数据，[1] new

    Data_Transmitter() = default;
    void get_data(unsigned rs, unsigned &ret) {}   
    void save(unsigned rd, unsigned val, Stages stage) {}
    void update(bool EXE_stall, bool MEM_stall) {} // 旧 <- 新
    
    void display() {}
};
```

#### `cpu.hpp`

封装上述的模拟元件，作为主程序的接口

```cpp
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

    Branch_Predictor predictor; // 分支预测
    Data_Transmitter transmitter; //数据传递

    int data_hazard_cnt = 0;
    
    CPU() = default;
    static unsigned int hex_to_dec(const string &s) {}
    void input(){}  //读入所有数据
    static inline unsigned sign_extend(const unsigned &x, const int &cur_len) {} //符号位扩展
    
    void instruction_fetch() {}
    void instruction_decode() {}
    void execute() {}
    void memory_access() {}
    void write_back_to_register() {}
    
    void print() {}
    
    void pass_cycle() {} //用上一个周期的输出来更新下一个周期
    void judge() {} //进行停顿、数据传递等
    
    void run() {
        while (EXE.ins.name != END) {
            pass_cycle(); //读入上一个周期的输出
            
            //倒序执行，实现伪并行
            write_back_to_register();
            memory_access();
            execute();
            instruction_decode();
            instruction_fetch();
            
            clk.timing(); // 时钟工作
            judge(); //进行更新与特判
            
            regs.reg[0] = 0; //每次操作后都要手动清空
        }
        printf("%u\n", regs.reg[10] & 255u); //第10个寄存器，作为输出位，保存程序的返回结果
    }
};
```