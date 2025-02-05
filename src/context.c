/*
 * Copyright (c) 2025 windflower
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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