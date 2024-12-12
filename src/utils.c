#include "utils.h"

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
    }
}

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
void heap_push(Heap *heap, int weight, void *data) {
    if (heap->size == heap->capacity) {
        heap->capacity *= 2;
        heap->nodes = (HeapNode *)realloc(heap->nodes, heap->capacity * sizeof(HeapNode));
    }

    int i = ++heap->size;
    heap->nodes[i].w = weight;
    heap->nodes[i].data = data;

    // 上浮操作，保持堆的性质
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
    return NULL;  // 堆为空时返回 NULL
}
HeapNode *heap_pop(Heap *heap) {
    if (heap->size == 0) {
        return NULL;
    }

    // 交换堆顶和最后一个元素
    HeapNode *minNode = &heap->nodes[1];
    heap->nodes[1] = heap->nodes[heap->size--];

    // 堆化操作
    heap_pushdown(heap, 1);

    return minNode;
}
void heap_pushdown(Heap *heap, int p) {
    int smallest = p;
    int left = 2 * p;
    int right = 2 * p + 1;

    // 找到最小值的节点
    if (left < heap->size && heap->nodes[left].w < heap->nodes[smallest].w) {
        smallest = left;
    }
    if (right < heap->size && heap->nodes[right].w < heap->nodes[smallest].w) {
        smallest = right;
    }

    // 如果最小值不是当前节点，交换并递归堆化
    if (smallest != p) {
        HeapNode tmp = heap->nodes[p];
        heap->nodes[p] = heap->nodes[smallest];
        heap->nodes[smallest] = tmp;
        heap_pushdown(heap, smallest);
    }
}
