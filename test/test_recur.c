#include <stdio.h>

#include "coheader.h"
#include "coroutine.h"
int recnum = 0;
void *auto_rec(const void *) {
    if (recnum >= 5) return NULL;
    char name[2];
    name[0] = 'a';
    name[1] = '\0';
    name[0] += recnum;
    recnum++;
    printf("co %s begin\n", name);
    coroutine_t co1, co2;
    co1 = coroutine_create(auto_rec, NULL, 0);
    co2 = coroutine_create(auto_rec, NULL, 0);
    coroutine_join(co1);
    coroutine_join(co2);
    printf("co %s finished\n", name);
    return NULL;
}
int main() {
    printf("***************test auto rec**************\n");
    auto_rec(NULL);
}
/*正确输出：
***************test auto rec**************
co a begin
co a finished
co b begin
co b finished
co c begin
co c finished
co d begin
co d finished
co e begin
co e finished

*/