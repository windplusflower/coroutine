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