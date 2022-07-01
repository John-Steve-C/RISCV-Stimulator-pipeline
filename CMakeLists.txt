cmake_minimum_required(VERSION 3.16)
project(RISC_V)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(code
        main.cpp
        src/cpu.hpp
        src/component.hpp
        src/Instruction.hpp
#        Reference/1_flow_fast.cpp
        )
