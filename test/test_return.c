#include <assert.h>
#include <stdlib.h>
#include "coheader.h"
//自动调度
void* fib(const void* arg) {
    int n = *(int*)arg;
    //返回值必须分配在堆上
    int* res = (int*)malloc(sizeof(int));
    *res = 1;
    if (n == 1 || n == 2) return res;
    int arg1 = n - 1, arg2 = n - 2;
    coroutine_t f1 = coroutine_create(fib, &arg1, 0);
    coroutine_t f2 = coroutine_create(fib, &arg2, 0);
    int *res1 = coroutine_join(f1), *res2 = coroutine_join(f2);
    *res = *res1 + *res2;
    free(res1);
    free(res2);
    return (void*)res;
}
//手动调度
void* fib2(const void* arg) {
    int n = *(int*)arg;
    //返回值必须分配在堆上
    int* res = (int*)malloc(sizeof(int));
    *res = 1;
    if (n == 1 || n == 2) return res;
    int arg1 = n - 1, arg2 = n - 2;
    coroutine_t f1 = coroutine_create(fib2, &arg1, 0);
    coroutine_t f2 = coroutine_create(fib2, &arg2, 0);
    coroutine_resume(f1);
    coroutine_resume(f2);
    int *res1 = coroutine_get_return_val(f1), *res2 = coroutine_get_return_val(f2);
    *res = *res1 + *res2;
    free(res1);
    free(res2);
    coroutine_free(f1);
    coroutine_free(f2);
    return (void*)res;
}
int main() {
    printf("input n:");
    int n;
    scanf("%d", &n);
    coroutine_t f = coroutine_create(fib, &n, 0);
    int* res1 = (int*)coroutine_join(f);
    int* res2 = fib2(&n);
    assert(*res1 == *res2);
    printf("f(%d) is %d\n", n, *res1);
}
/*
输入: n  输出:斐波那契数列第n项
比如：
input n:21
f(21) is 10946
*/