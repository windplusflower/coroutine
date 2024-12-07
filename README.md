# coroutine
a coroutine framework written in C

## 进度
- **24.11.23**: 实现简易的单对父子协程切换功能。
- **24.11.26**: 实现先进先出协程调度器对协程进行自动调度。
- **24.11.28**: 不在事件循环里对主协程特殊处理，而是作为普通协程受调度器调度；提供开启事件循环的接口，改为手动开启事件循环。
- **24.11.30**: 添加协程调用栈的支持，为手动调度的支持做准备。
- **24.12.03**: 添加外部日志库用于调试，尝试支持手动调度。
- **24.12.04**: 实现手动调度与自动调度混合；对用户函数进行封装使得函数退出时能自动修改status为DEAD。
- **24.12.06**: 手写汇编实现get_context,set_context以及swap_context用来替换posix里相关的函数，居然一次性写对了。

## Debug 记录
### 2024.11.25~2024.11.26
将调度方式切换为协程统一返回至事件循环，由事件循环进行resume后，当事件循环进行resume时会segment fault。调试后发现是因为为保存栈空间分配的1024B不够，改成128*1024B后成功运行。
### 2024.11.29~2024.11.30
添加协程调用栈的支持，遇到了segment fault，发现是因为在yield中获取当前协程时，先弹出调用栈，再获取栈顶元素导致的错误。
### 2024.12.4
将函数进行一次封装后，调试输出发现函数的arg参数为null。后发现是因为eventloop也调用coroutine_init进行创建，但有些操作并不适用于eventloop，因此单独实现epoll_event的初始化以解决。
