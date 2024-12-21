#include "context.h"

#include <string.h>

#include "log.h"

//生成上下文
void make_context(Context* context, void (*func)()) {
    //计算栈底位置，令sp指向栈底
    char* sp = context->ss_sp + context->ss_size - sizeof(void*);
    // x86_64要求sp对16字节对齐
    sp = (char*)((unsigned long)sp & -16L);
    //存放返回地址，ret指令会跳转到当前sp所指地址
    void** ret_addr = (void**)sp;
    *ret_addr = (void*)func;

    //初始化通用寄存器
    memset(context->regs, 0, sizeof(context->regs));
    //初始化浮点寄存器
    memset(context->fregs, 0, sizeof(context->fregs));
    //初始化栈指针
    context->regs[RSP] = sp;
    //初始化标志寄存器
    context->rflags = (void*)2;
    return;
}