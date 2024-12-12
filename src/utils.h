#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//设置日志级别
void log_set_level_from_env();

// 小根堆
typedef struct {
    int w;
    void *data;
} HeapNode;
typedef struct {
    HeapNode *nodes;
    int size;
    int capacity;
} Heap;
Heap *heap_create(int capacity);
void heap_free(Heap *heap);
void heap_push(Heap *heap, int weight, void *data);
HeapNode *heap_top(Heap *heap);
HeapNode *heap_pop(Heap *heap);
void heap_pushdown(Heap *heap, int index);
void swap(HeapNode *a, HeapNode *b);

#endif