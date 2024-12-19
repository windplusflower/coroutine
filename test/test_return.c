#include <stdlib.h>
#include "coheader.h"
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
int main() {
    printf("input n:");
    int n;
    scanf("%d", &n);
    coroutine_t f = coroutine_create(fib, &n, 0);
    printf("f(%d) is %d\n", n, *(int*)coroutine_join(f));
}
/*
输入: n  输出:斐波那契数列第n项
*/