#include <stdlib.h>
#include "coheader.h"
void* sum(const void* arg) {
    int n = *(int*)arg;
    int* res = malloc(sizeof(int));
    *res = n;
    if (n == 1) return res;
    n--;
    coroutine_t co = coroutine_create(sum, &n, 0);
    coroutine_resume(co);
    int* val = coroutine_get_return_val(co);
    coroutine_free(co);
    *res += *val;
    free(val);
    return res;
}
int main() {
    int n;
    printf("please input n:");
    scanf("%d", &n);
    int ans = *(int*)sum(&n);
    printf("sum from 1 to %d is %d\n", n, ans);
}