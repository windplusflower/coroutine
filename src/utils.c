#include "utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
//通过环境变量设置日志输出级别
void log_set_level_from_env() {
    const char *log_level = getenv("LOG_LEVEL");
    if (log_level != NULL) {
        if (strcmp(log_level, "LOG_TRACE") == 0) {
            log_set_level(LOG_TRACE);
        } else if (strcmp(log_level, "LOG_DEBUG") == 0) {
            log_set_level(LOG_DEBUG);
        } else if (strcmp(log_level, "LOG_INFO") == 0) {
            log_set_level(LOG_INFO);
        } else if (strcmp(log_level, "LOG_WARN") == 0) {
            log_set_level(LOG_WARN);
        } else if (strcmp(log_level, "LOG_ERROR") == 0) {
            log_set_level(LOG_ERROR);
        } else if (strcmp(log_level, "LOG_FATAL") == 0) {
            log_set_level(LOG_FATAL);
        }
    }else log_set_level(LOG_INFO);
}

//小根堆
Heap *heap_create(int capacity) {
    Heap *heap = (Heap *)malloc(sizeof(Heap));
    heap->size = 0;
    heap->capacity = capacity;
    heap->nodes = (HeapNode *)malloc(capacity * sizeof(HeapNode));
    return heap;
}
void heap_free(Heap *heap) {
    if (heap) {
        free(heap->nodes);
        free(heap);
    }
}
void heap_push(Heap *heap, long long weight, void *data) {
    if (heap->size == heap->capacity) {
        heap->capacity *= 2;
        heap->nodes = (HeapNode *)realloc(heap->nodes, heap->capacity * sizeof(HeapNode));
    }

    int i = ++heap->size;
    heap->nodes[i].w = weight;
    heap->nodes[i].data = data;

    while (i > 1 && heap->nodes[i].w < heap->nodes[i >> 1].w) {
        HeapNode tmp = heap->nodes[i];
        heap->nodes[i] = heap->nodes[i >> 1];
        heap->nodes[i >> 1] = tmp;
        i >>= 1;
    }
}
HeapNode *heap_top(Heap *heap) {
    if (heap->size > 0) {
        return &heap->nodes[1];
    }
    return NULL;
}
HeapNode heap_pop(Heap *heap) {
    assert(heap->size > 0);

    HeapNode minNode = heap->nodes[1];
    heap->nodes[1] = heap->nodes[heap->size--];

    heap_pushdown(heap, 1);

    return minNode;
}
void heap_pushdown(Heap *heap, int p) {
    int smallest = p;
    int left = 2 * p;
    int right = 2 * p + 1;

    if (left < heap->size && heap->nodes[left].w < heap->nodes[smallest].w) {
        smallest = left;
    }
    if (right < heap->size && heap->nodes[right].w < heap->nodes[smallest].w) {
        smallest = right;
    }

    if (smallest != p) {
        HeapNode tmp = heap->nodes[p];
        heap->nodes[p] = heap->nodes[smallest];
        heap->nodes[smallest] = tmp;
        heap_pushdown(heap, smallest);
    }
}

bool heap_isempty(Heap *heap) {
    return heap->size == 0;
}

//超时
void set_timeout(struct timeval *dest, struct timeval *src) {
    *dest = *src;
    dest->tv_usec += 1000;
}

int get_timeout(struct timeval *t) {
    return t->tv_sec * 1000 + t->tv_usec / 1000 - 1;
}

long long get_now() {
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000ll + now.tv_usec / 1000;
}

//其它
unsigned long long min(unsigned long long x, unsigned long long y) {
    return x < y ? x : y;
}
