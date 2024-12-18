#ifndef CONTEXT_H
#define CONTEXT_H

#include <stddef.h>
typedef enum {
    RAX = 0,
    RBX = 1,
    RCX = 2,
    RDX = 3,
    RSI = 4,  //第二个参数
    RDI = 5,  //第一个参数
    RBP = 6,  //栈帧
    RSP = 7,  //栈指针
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
} Reg;
typedef struct Context {
    void* regs[16];
    char* ss_sp;
    size_t ss_size;
} Context;

void make_context(Context* context, void (*func)());
extern void swap_context(Context*, Context*);
#endif