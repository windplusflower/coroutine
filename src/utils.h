#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
//设置日志级别
void log_set_level_from_env();

// 小根堆
typedef struct {
    long long w;
    void *data;
} HeapNode;
typedef struct {
    HeapNode *nodes;
    int size;
    int capacity;
} Heap;
typedef struct HandleTable {
    void **table;
    //未被使用的句柄
    int *unused;
    // size表示unused的大小
    int size, capacity;
} HandleTable;

Heap *heap_create(int capacity);
void heap_free(Heap *heap);
void heap_push(Heap *heap, long long weight, void *data);
HeapNode *heap_top(Heap *heap);
HeapNode heap_pop(Heap *heap);
void heap_pushdown(Heap *heap, int index);
bool heap_isempty(Heap *heap);

//超时
void set_timeout(struct timeval *dest, struct timeval *src);
int get_timeout(struct timeval *t);
long long get_now();

//其它
unsigned long long min(unsigned long long x, unsigned long long y);

void ut_init_handle_table(HandleTable* table);
int ut_alloc_id(HandleTable* table) ;
void *ut_get_item_by_id(HandleTable* table,int id) ;
void ut_free_id(HandleTable*table,int id);
#endif