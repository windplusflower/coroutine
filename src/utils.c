/*
 * Copyright (c) 2025 windflower
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "utils.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#define TABLESIZE 1024
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
    } else
        log_set_level(LOG_INFO);
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
    if (heap->size == heap->capacity - 1) {
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

//初始化hanlde与item之间的映射表，可动态扩容
void ut_init_handle_table(HandleTable *table) {
    table->capacity = TABLESIZE;
    table->size = TABLESIZE;
    table->table = (void **)calloc(TABLESIZE, sizeof(void *));
    table->unused = (int *)malloc(TABLESIZE * sizeof(int));
    for (int i = 0; i < TABLESIZE; i++) table->unused[i] = i;
}

//分配Handle
int ut_alloc_id(HandleTable *table) {
    if (table->size == 0) {
        int n = table->capacity;
        table->capacity = n * 2;
        table->size = n;
        table->table = realloc(table->table, n * 2 * sizeof(void *));
        //虽然现在unused只需要n的空间，但是后续可能会有新的句柄从co_table中释放，最大可以到2*n
        table->unused = realloc(table->unused, n * 2 * sizeof(int));
        for (int i = n; i < n * 2; i++) table->unused[i - n] = i;
    }
    table->size--;
    return table->unused[table->size];
}

//根据handle获取item
void *ut_get_item_by_id(HandleTable *table, int id) {
    if (id >= table->capacity) return NULL;
    return table->table[id];
}

//释放Handle
void ut_free_id(HandleTable *table, int id) {
    assert(table->table[id] != NULL);
    table->table[id] = NULL;
    table->unused[table->size++] = id;
}

//创建空链表
CoList *make_empty_list() {
    CoList *res = (CoList *)malloc(sizeof(CoList));
    res->head = (CoNode *)calloc(1, sizeof(CoNode));
    res->tail = res->head;
    return res;
}

//生成结点
CoNode *make_node(void *data) {
    CoNode *res = (CoNode *)calloc(1, sizeof(CoNode));
    res->data = data;
    res->valid = 1;
    res->free_times = 1;
    return res;
}

//不需要释放co指向的内存，因为会转移这块内存的所有权，它之后还要用到，在协程结束时才由调度器销毁。
void free_node(CoNode *node) {
    node->free_times--;
    assert(node->free_times >= 0);
    //只要释放一次后，就一定是无效的了
    node->valid = 0;
    if (node->free_times == 0) free(node);
}

void free_list(CoList *list) {
    while (!is_emptylist(list)) pop_front(list);
    free(list->head);
    free(list);
}

//添加到链表尾部
CoNode *push_back(CoList *list, void *data) {
    CoNode *node = make_node(data);
    list->tail->next = node;
    list->tail = node;
    return node;
}

//从链表头部弹出，跳过无效结点，空时返回NULL
void *pop_front(CoList *list) {
    CoNode *node;
    void *data = NULL;
    while (list->head->next && !list->head->next->valid) {
        node = list->head->next;
        list->head->next = node->next;
        free_node(node);
    }
    if (list->head->next) {
        node = list->head->next;
        list->head->next = node->next;
        data = node->data;
        free_node(node);
    }
    if (list->head->next == NULL) list->tail = list->head;
    return data;
}

//移除node的下一个节点
void remove_next(CoList *list, CoNode *node) {
    assert(node->next != NULL);
    CoNode *tmp = node->next;
    node->next = tmp->next;
    free_node(tmp);
    if (node->next == NULL) list->tail = node;
}

//判断链表是否为空
bool is_emptylist(CoList *list) {
    return list == NULL || list->head == list->tail;
}