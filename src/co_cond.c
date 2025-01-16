#include "co_cond.h"
#include "coroutine.h"
#include "event_manager.h"
#include "log.h"
#include "utils.h"
//初始化hanlde与cond之间的映射表，可动态扩容
void init_cond_table() {
    ut_init_handle_table(&COND_TABLE);
}

//分配Handle
int alloc_cond_id() {
    return ut_alloc_id(&COND_TABLE);
}

//根据handle获取Cond
Cond* get_cond_by_id(int id) {
    return ut_get_item_by_id(&COND_TABLE, id);
}

//释放Handle
void free_cond_id(int id) {
    ut_free_id(&COND_TABLE, id);
}

int co_cond_alloc() {
    eventloop_init();
    Cond* cond = (Cond*)malloc(sizeof(Cond));
    cond->list = make_empty_list();
    int handle = alloc_cond_id();
    COND_TABLE.table[handle] = cond;
#ifdef USE_DEBUG
    log_debug("alloc cond %d", handle);
#endif
    return handle;
}

//通知一个
void co_cond_signal(int handle) {
    Cond* cond = get_cond_by_id(handle);
    if (cond == NULL) {
        log_error("cond %d not exist!", handle);
        return;
    }
    //移除已经因超时而无效的协程
    while (!is_emptylist(cond->list) && !cond->list->head->next->valid)
        remove_next(cond->list, cond->list->head);
    if (is_emptylist(cond->list)) return;
    cond->list->head->next->valid = 0;
    Coroutine* co = pop_front(cond->list);
#ifdef USE_DEBUG
    log_debug("signal coroutine %s", co->name);
#endif
    add_coroutine(co);
}

void co_cond_broadcast(int handle) {
    Cond* cond = get_cond_by_id(handle);
    if (cond == NULL) {
        log_error("cond %d not exist!", handle);
        return;
    }
    while (!is_emptylist(cond->list)) {
        if (!cond->list->head->next->valid)
            remove_next(cond->list, cond->list->head);
        else {
            cond->list->head->next->valid = 0;
            Coroutine* co = pop_front(cond->list);
            add_coroutine(co);
        }
    }
}

//时间毫秒,返回是否成功等到
bool co_cond_wait(int handle, int timeout) {
    Cond* cond = get_cond_by_id(handle);
    if (cond == NULL) {
        log_error("cond %d not exist!", handle);
        return -1;
    }
    push_back(cond->list, get_current_coroutine());
    return wait_cond(cond->list->tail, timeout);
}

//正在使用的话会返回-1;
int co_cond_free(int handle) {
    Cond* cond = get_cond_by_id(handle);
    if (cond == NULL) {
        log_error("cond %d not exist!", handle);
        return -1;
    };
    if (!is_emptylist(cond->list)) return -1;
    free_list(cond->list);
    free(cond);
    free_cond_id(handle);
    return 0;
}