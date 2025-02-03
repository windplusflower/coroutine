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

//从链表头部弹出
void *pop_front(CoList *list) {
    assert(list->head->next != NULL);
    CoNode *node = list->head->next;
    list->head->next = node->next;
    void *data = node->data;
    if (list->head->next == NULL) list->tail = list->head;
    free_node(node);
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

//无锁队列

// 初始化队列
void lock_free_list_init(LockFreeList *list) {
    LockFreeNode *dummy = malloc(sizeof(LockFreeNode));
    atomic_init(&dummy->next, NULL);
    dummy->data = NULL;

    atomic_init(&list->head, dummy);
    atomic_init(&list->tail, dummy);
}

// 线程安全的入队操作
void *lk_push_back(LockFreeList *list, void *data, int free_times) {
    LockFreeNode *new_node = malloc(sizeof(LockFreeNode));
    new_node->data = data;
    atomic_init(&new_node->next, NULL);
    atomic_init(&new_node->free_times, free_times);

    LockFreeNode *tail;
    LockFreeNode *next;

    while (1) {
        tail = atomic_load_explicit(&list->tail, memory_order_acquire);
        next = atomic_load_explicit(&tail->next, memory_order_acquire);

        // 验证一致性视图
        if (tail == atomic_load_explicit(&list->tail, memory_order_relaxed)) {
            if (next == NULL) {
                // CAS原子追加新节点
                if (atomic_compare_exchange_weak_explicit(
                        &tail->next, &next, new_node,
                        memory_order_release,     // 成功时的内存序
                        memory_order_relaxed)) {  // 失败时的内存序
                    break;
                }
            } else {
                // 帮助推进尾指针
                atomic_compare_exchange_weak_explicit(&list->tail, &tail, next,
                                                      memory_order_release, memory_order_relaxed);
            }
        }
    }

    // 尝试推进尾指针（允许失败）
    atomic_compare_exchange_weak_explicit(&list->tail, &tail, new_node, memory_order_release,
                                          memory_order_relaxed);
    return new_node;
}

void lk_free_node(LockFreeNode *node) {
    int free_times = atomic_fetch_sub(&node->free_times, 1) - 1;
    atomic_store(&node->valid, 0);
    if (free_times == 0) free(node);
}
// 线程安全的出队操作（返回NULL表示空队列）
void *lk_pop_front(LockFreeList *list) {
    LockFreeNode *head;
    LockFreeNode *tail;
    LockFreeNode *next;
    void *data = NULL;

    while (1) {
        head = atomic_load_explicit(&list->head, memory_order_acquire);
        tail = atomic_load_explicit(&list->tail, memory_order_acquire);
        next = atomic_load_explicit(&head->next, memory_order_acquire);

        // 一致性检查
        if (head == atomic_load_explicit(&list->head, memory_order_relaxed)) {
            if (head == tail) {
                if (next == NULL) {
                    return NULL;  // 空队列
                }
                // 帮助推进尾指针
                atomic_compare_exchange_weak_explicit(&list->tail, &tail, next,
                                                      memory_order_release, memory_order_relaxed);
            } else {
                data = next->data;
                // CAS移动头指针
                if (atomic_compare_exchange_weak_explicit(
                        &list->head, &head, next, memory_order_release, memory_order_relaxed)) {
                    break;
                }
            }
        }
    }

    // 安全延迟释放（需配合内存回收方案）
    lk_free_node(head);  // 警告：实际使用时需要处理ABA问题
    return data;
}