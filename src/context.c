#include "context.h"

#include <string.h>

#include "log.h"

//生成上下文
void make_context(Context* context, void (*func)()) {
    //计算栈底位置，令sp指向栈底
    char* sp = context->ss_sp + context->ss_size - sizeof(void*);
    sp = (char*)((unsigned long)sp & -16L);
    //存放返回地址，ret指令会跳转到当前sp所指地址
    void** ret_addr = (void**)sp;
    *ret_addr = (void*)func;

    //初始化寄存器
    memset(context->regs, 0, sizeof(context->regs));
    context->regs[RSP] = sp;
    return;
}