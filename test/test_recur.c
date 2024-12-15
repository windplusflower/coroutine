#include <stdio.h>

#include "coroutine.h"
#include "utils.h"
int recnum = 0;
void auto_rec(void *) {
    if (recnum >= 5) return;
    char name[2];
    name[0] = 'a';
    name[1] = '\0';
    name[0] += recnum;
    recnum++;
    printf("co %s begin\n", name);
    Coroutine co1, co2;
    coroutine_init(&co1, auto_rec, NULL, 0);
    coroutine_init(&co2, auto_rec, NULL, 0);
    printf("co %s finished\n", name);
    while (co1.status != COROUTINE_DEAD) coroutine_yield();
    while (co2.status != COROUTINE_DEAD) coroutine_yield();
}
void hand_rec(void *depth) {
    int d = *(int *)depth;
    if (d >= 3) return;
    char name[2];
    name[0] = '0';
    name[1] = '\0';
    name[0] += d;
    printf("co depth %s begin\n", name);
    Coroutine co1, co2;
    d++;
    coroutine_init(&co1, hand_rec, &d, 0);
    coroutine_init(&co2, hand_rec, &d, 0);
    coroutine_resume(&co1);
    coroutine_resume(&co2);
    printf("co depth %s finished\n", name);
}
int main() {
    log_set_level_from_env();
    start_eventloop();
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