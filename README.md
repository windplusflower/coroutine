# coroutine
一个纯C实现的基于异步IO事件通知的协程框架

## 本协程库特点
1. 协程可嵌套、可递归，不限深度，只要内存够。
2. 支持非对称式的手动调度和对称式的自动调度混用。
3. 支持协程版本的函数与系统原版函数混用，即协程并不会对其它函数的行为产生影响。
4. 类似linux多线程，协程创建后只返回一个int型的句柄，向用户屏蔽内部结构。
5. 只需要包含一个头文件。
6. 实现hook机制，开启hook后即可像使用原版函数一样使用各函数。
7. 使用小根堆来实现超时机制。
8. 协程创建后即投入运行，不需要手动启动，不需要手动开启事件循环，支持获取返回值，使协程的使用方式更接近linux线程。

## 运行
使用`make testname`即可运行相应的test

可以使用`LOG_LEVEL`环境变量指定日志级别，不指定则默认是`LOG_INFO`

比如：
```
make test_rdwr LOG_LEVEL=LOG_DEBUG
```

## 接口
- 手动调度与自动调度通用：
```C
/*
创建协程，返回句柄;
参数func需要是一个void (*)(const void*)类型的函数;
arg是一个const void*指针，表示传给func的参数，可以为NULL;
stack_size是栈大小，可以用0表示由框架指定;
创建后的协程默认自动调度，当对协程显式使用coroutine_resume()启动后会变为手动调度。
*/
coroutine_t coroutine_create(void (*func)(const void *), void *arg, size_t stack_size);

//挂起当前协程
void coroutine_yield();

//强制结束协程
//手动调度的协程结束后需要用coroutine_free回收
//自动调度的协程不需要手动回收
void coroutine_cancel(coroutine_t handle);

//判断协程是否处于已结束未回收的状态
//自动调度已经结束但还未调用join，或者手动调度已经结束但还未调用free
bool coroutine_is_finished(coroutine_t handle);

//开启hook机制
void enable_hook();

//关闭hook机制
void unable_hook();

//检查是否开启了hook
bool is_hook_enabled();

// 支持hook的函数：read,write,send,recv,sendto,recvfrom,accept,connect,setsockopt,sleep,usleep
```
- 手动调度专用：
```C
//唤醒协程co，被唤醒的协程挂起后会回到本协程
void coroutine_resume(coroutine_t handle);

//获取协程返回值
//需要协程运行完毕后才能使用
void* coroutine_get_return_val(coroutine_t handle);

//释放已经结束的手动调度的协程内存
void coroutine_free(coroutine_t handle);
```
- 自动调度专用：
```C
//等待协程运行结束并释放协程内存
void coroutine_join(coroutine_t handle);
```

## 示例
- 使用：src目录下会被编译为共享库libsrc.so，头文件包含coheader.h，链接此共享库即可使用
- 手动调度：
```C
#include "coheader.h"
void func(const void* arg){
    printf("A\n");
    coroutine_yield();
    printf("C\n");
}
int main(){
    coroutine_t co=coroutine_create(func,NULL,0);
    coroutine_resume(co);
    printf("B\n");
    coroutine_resume(co);
    coroutine_free(co);
}
/*输出：
A
B
C
*/
```
- 自动调度：
```C
#include "coheader.h"
void func(const void* arg){
    printf("A\n");
    coroutine_yield();
    printf("B\n");
}
int main(){
    coroutine_t co=coroutine_create(func,NULL,0);
    coroutine_join(co);
    printf("C\n");
}
/*输出：
A
B
C
*/
```

## 进度
- **24.11.23**: 实现简易的单对父子协程切换功能。
- **24.11.26**: 实现先进先出协程调度器对协程进行自动调度。
- **24.11.28**: 不在事件循环里对主协程特殊处理，而是作为普通协程受调度器调度；提供开启事件循环的接口，改为手动开启事件循环。
- **24.11.30**: 添加协程调用栈的支持，为手动调度的支持做准备。
- **24.12.03**: 添加外部日志库`log.c`用于调试，尝试支持手动调度。
- **24.12.04**: 实现手动调度与自动调度混合；对用户函数进行封装使得函数退出时能自动修改status为DEAD。
- **24.12.06**: 手写汇编实现get_context,set_context以及swap_context用来替换ucontext的相关的函数，居然一次性写对了。
- **24.12.07**: 为进程调度引入epoll，准备实现各调用的协程版本。
- **24.12.09**: 修复epoll相关bug，完善基于epoll的协程调度，实现可自动在阻塞时让出cpu的read调用。
- **24.12.10**: 修复epoll相关bug，实现write调用。
- **24.12.11**: 实现sendto、recvfrom、send、recv、accept。
- **24.12.12**: 修复创建协程传递的参数不能为NULL的BUG，修复epoll相关bug，添加小根堆的实现，为实现超时机制做准备。完善readme文档介绍。
- **24.12.14**: 重构wait_event，添加之前系统调用的超时机制，添加sleep和usleep。
- **24.12.16**: 实现协程版本的connect，实现协程调用栈的动态扩容。
- **24.12.18**: 在考虑协程结构体本身内存是由用户释放还是框架释放时遇到了两难，后来想到可以模仿linux线程，只给用户提供int类型的句柄，这样不但可以防止用户以外释放内存，还可以向用户屏蔽协程内部结构。实现了单头文件，现在只需要包含coheader.h文件即可使用。实现了hook机制。模仿linux多线程，协程创建即投入使用，无需手动启动，使用coroutine_join回收等待。
- **24.12.19**: 支持协程返回值；完善接口和注释。
- **24.12.20**: 修改join的机制，从轮询改为通知，减少协程切换的次数；修复对自动调度的协程调用cancel会发生内存泄漏的bug；添加接口用于查询协程是否结束；完善测例。

## Debug 记录
### 2024.11.25~2024.11.26
- 将调度方式切换为协程统一返回至事件循环，由事件循环进行resume后，当事件循环进行resume时会segment fault。调试后发现是因为为保存栈空间分配的1024B不够，改成128*1024B后成功运行。
### 2024.11.29~2024.11.30
- 添加协程调用栈的支持，遇到了segment fault，发现是因为在yield中获取当前协程时，先弹出调用栈，再获取栈顶元素导致的错误。
### 2024.12.4
- 将函数进行一次封装后，调试输出发现函数的arg参数为null。后发现是因为eventloop也调用coroutine_init进行创建，但有些操作并不适用于eventloop，因此单独实现epoll_event的初始化以解决。
### 2024.12.9
- 对于test_read，read第一次让出cpu后，协程一直在main和test_suspend之间切换，即便已经有了标准输入。调试发现是因为之前的逻辑只有在可执行协程为空时才会去检查epoll，但实际上有的可执行协程可能实在忙等待(比如此例的main)，从而永远无法调度到read。改为每次协程切换时都检查epoll后解决。
- 解决上述问题后，发现只有第一次read阻塞后能正常切换，当第二次read阻塞后就不会切换了。调试发现，read被多次加入可执行队列中，因为之前改成了每次切换协程时都检查epoll，但并没有修改epoll，导致read协程被多次添加到可执行队列。因此当read被从epoll加入到可执行队列时，需要移除对它的监听。进而想到，可能会出现多个协程监听同一个fd的不同事件的情况，这是之前没有考虑过的，因此重构epoll相关数据结构和代码逻辑，为每个文件描述符维护一个正在等待的协程队列，当某事件发生时，遍历这个文件描述符的协程队列，将等待对应事件的协程加入到可执行队列并从等待队列移除，此时如果还有其它事件需要继续监听，则修改监听事件，否则从epoll中删除这个文件描述符。
- 解决上述问题后，发现read协程还是有被多次加入可执行队列的现象。调试后发现，是因为wait_event中调用了coroutine_yield来让出cpu，但是这个操作本身就会把协程加入可执行队列。因此为协程新增了in_epoll字段，表示这个协程是否加入了epoll，如果是，那么yield时则不需要将其加入可执行队列。
### 2024.12.10
- 昨晚睡前突然想到，如果多个协程监听同一个fd的读操作，当fd可读时这些协程都会被加入可执行队列，此时如果第一个协程把fd读完了，那么它又会变回不可读，但后面的协程依旧是以为它可读，从而阻塞(其它操作同理)。想了两个方案，一个是在每次读操作之前再检查一下是否确实可读，另一个是对于同一个fd一次只唤醒一个协程。如果用前者的话，会有反复将协程加入移除epoll队列的情况，当监听同一个fd的协程较多时会浪费较多时间，所以选择了后者。
### 2024.12.11
- 运行co_recvfrom时，在本应阻塞的情况下返回了“Resource temporarily unavailable”导致没有让出cpu。调试后发现，是因为co_sendto时，需要将其改为非阻塞，但是在yield之前没有恢复阻塞，导致其它协程使用该fd时，它变成了非阻塞。在挂起之前将其改为阻塞解决。
### 2024.12.12
- 尝试把监听的事件添加上EPOLLHUP和EPOLLERR后，发现epoll不会对相应的EPOLLIN和EPOLLOUT进行响应。调试后发现，是因为epoll监听事件的删除逻辑中，是需要对应fd的events为0时才删除对应的fd的监听，但所有等待进程的响应事件的并集可能只是对应fd事件的真子集，此时等待队列为空，应该要触发fd的删除而没有删除。当再次为fd添加监听时，因为等待队列为空，所以会使用ADD来添加，但实际上这个fd的监听没有从epoll中移除，导致添加失败，实际上并没有监听EPOLLIN或EPOLLOUT。增加判断，在等待队列为空时也删除对fd的监听，问题解决。
### 2024.12.13
- 为框架添加超时机制后，自己写了个测试test_timeout，测试后发现当超时触发时，程序会segmentfault，调试后发现是事件节点多次释放的原因。原本等待队列遇到一个事件时，会把对应的事件节点释放，但添加超时机制后，这个新的节点也被超时的堆所持有。为了解决这个问题，给事件节点添加了freetimes字段，表示需要释放的次数，相当于一个简化版的针对特定情况的shared_ptr。同时还添加了valid字段，表示这个节点是否有效，当超时触发时，可以把对应节点修改为无效状态。
- 修复以上问题后，程序能多运行一会，但仍有概率段错误。调试后发现，一个Heap节点对应的co成员的值会无故发生改变，进一步调试后发现是heap_pop返回了HeapNode指针导致的，对于pop应该返回完整的拷贝，修改后解决问题。
### 2024.12.15
- 写了一个测试test_recur来测试嵌套创建功能，出现了Segmentfault，观察日志后发现是已结束的协程仍被加入了可执行队列导致的，查看代码后发现是因为协程运行结束后会先修改状态为DEAD，然后调用coroutine_yield来让出CPU。但是coroutine_yield内部会直接将状态修改为SUSPEND，从而导致eventloop没有正确移除协程。最后添加了coroutine_finish函数来用作退出协程的接口，同时这个接口也可以开放给用户使用（虽然效果跟在协程函数里直接return是一样的）。
### 2024.12.17
- 添加hook机制，发现RTLD_NEXT宏无法找到导致编译失败，但是正确包含了dlfcn.h，其中也能看到RTLD_NEXT的定义。观察代码后发现它需要在定义了__USE_GNU宏时才会启用，而本项目并没有__USE_GNU宏。查阅资料后发现，__USE_GNU宏并没有开放给用户使用，如果需要定义，则需要定义_GNU_SOURCE宏来间接开启__USE_GNU。添加_GNU_SOUCR宏后问题解决。