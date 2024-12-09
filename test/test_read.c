#include "coroutine.h"
#include "hook.h"
#include "log.h"
#include "utils.h"

char buf[1024];
void read_test(void *) {
    while (1) {
        printf("please input:\n");
        co_read(0, buf, 1024);
        printf("Read from cmd:%s\n", buf);
    }
}
void testing_read_suspend(void *) {
    while (1) {
        printf("read was not running now!\n");
        sleep(1);
        coroutine_yield();
    }
}
int main() {
    log_set_level_from_env();
    log_debug("start");
    Coroutine reading, suspending;
    //参数并不使用，只是便于调试
    coroutine_init(&reading, read_test, "read_test", STACKSIZE);
    coroutine_init(&suspending, testing_read_suspend, "testing_suspend", STACKSIZE);
    start_eventloop();
    while (1) coroutine_yield();
    return 0;
}
/*正确输出：
please input:
然后每隔一秒输出read was not running now!
当键盘输入字符串s时会输出
Read from cmd:s
please input:
之后继续每隔一秒输出read was not running now!
如此循环
*/