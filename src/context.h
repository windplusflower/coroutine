#ifndef CONTEXT_H
#define CONTEXT_H

#include <stddef.h>
typedef enum {
    RBX = 0,
    RBP = 1,  // 栈帧
    RSP = 2,  // 栈指针
    R12 = 3,
    R13 = 4,
    R14 = 5,
    R15 = 6,
} Reg;

typedef struct Context {
    void* regs[7];
    char* ss_sp;
    size_t ss_size;
} Context;

void make_context(Context* context, void (*func)());
extern void swap_context(Context*, Context*);
#endif