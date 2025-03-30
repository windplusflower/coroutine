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

#include <assert.h>
#include <stdlib.h>
#include "coheader.h"
void *fib(void *arg) {
    int n = *(int *)arg;
    //返回值必须分配在堆上
    int *res = (int *)malloc(sizeof(int));
    *res = 1;
    if (n == 1 || n == 2) return res;
    int arg1 = n - 1, arg2 = n - 2;
    coroutine_t f1 = coroutine_create(fib, &arg1, 0);
    coroutine_t f2 = coroutine_create(fib, &arg2, 0);
    int *res1 = coroutine_join(f1), *res2 = coroutine_join(f2);
    *res = *res1 + *res2;
    free(res1);
    free(res2);
    return (void *)res;
}
int main() {
    printf("input n:");
    int n;
    scanf("%d", &n);
    coroutine_t f = coroutine_create(fib, &n, 0);
    int *res1 = (int *)coroutine_join(f);
    printf("f(%d) is %d\n", n, *res1);
}
/*
输入: n  输出:斐波那契数列第n项
比如：
input n:21
f(21) is 10946
*/