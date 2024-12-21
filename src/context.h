#ifndef CONTEXT_H
#define CONTEXT_H

#include <stddef.h>
#include <sys/cdefs.h>
typedef enum {
    RBX = 0,
    RBP = 1,
    RSP = 2,  // 栈指针
    R12 = 3,
    R13 = 4,
    R14 = 5,
    R15 = 6,
} Reg;

// fxsave浮点寄存器保存指令要求目的地址16字节对齐
// x86_64对sp指针也有对齐要求，将寄存器保存在栈上会有对齐的问题，因此分配堆来保存相关寄存器
typedef struct __attribute__((aligned(16))) Context {
    void* regs[7];
    void* rflags;
    char fregs[512];
    char* ss_sp;
    size_t ss_size;
} Context;

void make_context(Context* context, void (*func)());
extern void swap_context(Context*, Context*);
#endif