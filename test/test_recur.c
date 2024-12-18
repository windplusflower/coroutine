#include <stdio.h>

#include "coheader.h"
int recnum = 0;
void auto_rec(const void *) {
    if (recnum >= 5) return;
    char name[2];
    name[0] = 'a';
    name[1] = '\0';
    name[0] += recnum;
    recnum++;
    printf("co %s begin\n", name);
    coroutine_t co1, co2;
    co1 = coroutine_init(auto_rec, NULL, 0);
    co2 = coroutine_init(auto_rec, NULL, 0);
    coroutine_join(co1);
    coroutine_join(co2);
    printf("co %s finished\n", name);
}
void hand_rec(const void *depth) {
    int d = *(int *)depth;
    if (d >= 3) return;
    char name[2];
    name[0] = '0';
    name[1] = '\0';
    name[0] += d;
    printf("co depth %s begin\n", name);
    coroutine_t co1, co2;
    d++;
    co1 = coroutine_init(hand_rec, &d, 0);
    co2 = coroutine_init(hand_rec, &d, 0);
    coroutine_resume(co1);
    coroutine_resume(co2);
    printf("co depth %s finished\n", name);
}
int main() {
    printf("***************test auto rec**************\n");
    auto_rec(NULL);
    printf("\n***************test hand rec********************\n");
    int depth = 0;
    hand_rec(&depth);
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

***************test hand rec********************
co depth 0 begin
co depth 1 begin
co depth 2 begin
co depth 2 finished
co depth 2 begin
co depth 2 finished
co depth 1 finished
co depth 1 begin
co depth 2 begin
co depth 2 finished
co depth 2 begin
co depth 2 finished
co depth 1 finished
co depth 0 finished
*/