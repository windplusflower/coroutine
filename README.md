# coroutine
a coroutine framework written in C

## Debug 记录
### 2024.11.25~2024.11.26
将调度方式切换为协程统一返回至事件循环，由事件循环进行resume后，当事件循环进行resume时会segment fault。调试后发现是因为为保存栈空间分配的1024B不够，改成8*1024B后成功运行。
