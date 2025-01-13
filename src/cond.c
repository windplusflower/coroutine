#include"cond.h"
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
Cond *get_cond_by_id(int id) {
    return ut_get_item_by_id(&COND_TABLE, id);
}

//释放Handle
void free_cond_id(int id) {
    ut_free_id(&COND_TABLE,  id);
}