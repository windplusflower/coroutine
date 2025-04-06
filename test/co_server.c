#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include "coheader.h"
#define PORT 8080
#define BACKLOG 128
void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    // 接收请求（我们只处理简单请求，不关心具体内容）
    char buffer[128];
    // 构建 HTTP 响应头
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 2\r\n\r\n"
                           "OK";
    while (1) {
        int ret = recv(client_sock, buffer, sizeof(buffer), 0);
        if (ret <= 0) break;
        volatile int num = 1e5;
        while (num--)
            ;
        send(client_sock, response, strlen(response), 0);
    }
    // 关闭连接
    close(client_sock);

    return NULL;
}

int main() {
    enable_hook();
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    listen(server_sock, BACKLOG);
    while (1) {
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, NULL, NULL);
        coroutine_t co = coroutine_create(handle_client, client_sock, 0);
        coroutine_detach(co);
    }

    return 0;
}
