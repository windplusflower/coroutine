#include "coheader.h"

char buf[1024];
void read_test(const void *) {
    while (1) {
        printf("please input:\n");
        read(0, buf, 1024);
        printf("Read from cmd:%s\n", buf);
    }
}
void testing_read_suspend(const void *) {
    while (1) {
        printf("read was not running now!\n");
        co_sleep(1);
    }
}
int main() {
    enable_hook();
    coroutine_t reading, suspending;
    //参数并不使用，只是便于调试
    reading = coroutine_create(read_test, "read_test", 0);
    suspending = coroutine_create(testing_read_suspend, "testing_suspend", 0);
    coroutine_join(reading);
    coroutine_join(suspending);
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