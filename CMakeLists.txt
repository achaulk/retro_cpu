project(retrocpu)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

set(CPU_SOURCES
    cpu.cc
    jit.cc)
set(CPU_HEADERS
    host_system.h
    cpu.h
    jit.h)
add_library(cpu ${CPU_SOURCES} ${CPU_HEADERS})

set(CPU_65816_SOURCES
    cpu/65816/cpu_65c816.cc)
set(CPU_65816_HEADERS
    cpu/65816/cpu_65c816.h)
add_library(cpu_65816 ${CPU_65816_SOURCES} ${CPU_65816_HEADERS})
target_include_directories(cpu_65816 PUBLIC ./)

set(JIT_X64_SOURCES
    jit_x64/jit_x64.cc
    jit_x64/jit_x86_entrypoints.asm
    )
set(JIT_X64_HEADERS
    jit_x64/jit_x64.h
    jit_x64/x64_emitter.h)

add_library(jit_x64 ${JIT_X64_SOURCES})
add_dependencies(jit_x64 cpu)
target_include_directories(jit_x64 PUBLIC jit_x64 ./)

set(HOST_LINUX_SOURCES host/host_linux.cc)
add_library(host_linux ${HOST_LINUX_SOURCES})
add_dependencies(host_linux jit_x64 cpu)
target_include_directories(host_linux PUBLIC jit_x64 ./ host)
