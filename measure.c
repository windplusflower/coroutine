#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <wait.h>

// 运行程序并记录时间与内存
void run_program(const char *program) {
    struct timespec start_time, end_time;
    struct rusage usage;

    // 获取开始时间
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // 执行程序
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程执行指定程序
        execl(program, program, (char *)NULL);
        exit(0);
    } else {
        // 父进程等待子进程结束
        wait(NULL);
    }

    // 获取结束时间
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    // 获取资源使用情况（内存）
    getrusage(RUSAGE_CHILDREN, &usage);

    // 计算运行时间（纳秒）
    long long start_ns = start_time.tv_sec * 1000000000 + start_time.tv_nsec;
    long long end_ns = end_time.tv_sec * 1000000000 + end_time.tv_nsec;
    long long elapsed_ns = end_ns - start_ns;

    // 将时间转换为秒（带有纳秒小数）
    double elapsed_sec = (double)elapsed_ns / 1000000000.0;

    // 输出程序运行时间和内存使用
    printf("程序运行时间: %.9f 秒\n", elapsed_sec);
    printf("最大内存占用: %ld KB\n", usage.ru_maxrss); // ru_maxrss是以KB为单位
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "请提供要测试的程序路径\n");
        return 1;
    }

    run_program(argv[1]);

    return 0;
}
