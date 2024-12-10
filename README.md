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
- **24.12.07**: 为进程调度引入epoll，准备实现各调用的协程版本。
- **24.12.09**: 修复epoll相关bug，完善基于epoll的协程调度，实现可自动在阻塞时让出cpu的read调用。

## Debug 记录
### 2024.11.25~2024.11.26
将调度方式切换为协程统一返回至事件循环，由事件循环进行resume后，当事件循环进行resume时会segment fault。调试后发现是因为为保存栈空间分配的1024B不够，改成128*1024B后成功运行。
### 2024.11.29~2024.11.30
添加协程调用栈的支持，遇到了segment fault，发现是因为在yield中获取当前协程时，先弹出调用栈，再获取栈顶元素导致的错误。
### 2024.12.4
将函数进行一次封装后，调试输出发现函数的arg参数为null。后发现是因为eventloop也调用coroutine_init进行创建，但有些操作并不适用于eventloop，因此单独实现epoll_event的初始化以解决。
### 2024.12.9
- 对于test_read，read第一次让出cpu后，协程一直在main和test_suspend之间切换，即便已经有了标准输入。调试发现是因为之前的逻辑只有在可执行协程为空时才会去检查epoll，但实际上有的可执行协程可能实在忙等待（比如此例的main)，从而永远无法调度到read。改为每次协程切换时都检查epoll后解决。
- 解决上述问题后，发现只有第一次read阻塞后能正常切换，当第二次read阻塞后就不会切换了。调试发现，read被多次加入可执行队列中，因为之前改成了每次切换协程时都检查epoll，但并没有修改epoll，导致read协程被多次添加到可执行队列。因此当read被从epoll加入到可执行队列时，需要移除对它的监听。进而想到，可能会出现多个协程监听同一个fd的不同事件的情况，这是之前没有考虑过的，因此重构epoll相关数据结构和代码逻辑，为每个文件描述符维护一个正在等待的协程队列，当某事件发生时，遍历这个文件描述符的协程队列，将等待对应事件的协程加入到可执行队列并从等待队列移除，此时如果还有其它事件需要继续监听，则修改监听事件，否则从epoll中删除这个文件描述符。
- 解决上述问题后，发现read协程还是有被多次加入可执行队列的现象。调试后发现，是因为wait_event中调用了coroutine_yield来让出cpu，但是这个操作本身就会把协程加入可执行队列。因此为协程新增了in_epoll字段，表示这个协程是否加入了epoll，如果是，那么yield时则不需要将其加入可执行队列。
### 2024.12.10
昨晚睡前突然想到，如果多个协程监听同一个fd的读操作，当fd可读时这些协程都会被加入可执行队列，此时如果第一个协程把fd读完了，那么它又会变回不可读，但后面的协程依旧是以为它可读，从而阻塞(其它操作同理)。想了两个方案，一个是在每次读操作之前再检查一下是否确实可读，另一个是对于同一个fd一次只唤醒一个协程。如果用前者的话，会有反复将协程加入移除epoll队列的情况，当监听同一个fd的协程较多时会浪费较多时间，所以选择了后者。
