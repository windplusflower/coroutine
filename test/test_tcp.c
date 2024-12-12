#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "coroutine.h"
#include "hook.h"
#include "utils.h"

int PORT = 1000;
#define BUFFER_SIZE 1024

void send_coroutine(void* arg) {
    int server_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int client_sock;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return;
    }

    if (listen(server_sock, 3) < 0) {
        perror("Listen failed");
        return;
    }

    printf("Server is listening on port %d\n", PORT);
    if ((client_sock = co_accept(server_sock, (struct sockaddr*)&server_addr, &addr_len)) < 0) {
        perror("Accept failed");
        return;
    }
    printf("accept success\n");
    while (1) {
        snprintf(buffer, BUFFER_SIZE, "Message from server thread(%ld)", time(0));
        if (co_send(client_sock, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            break;
        }
        sleep(1);
        //这里的sleep不会让出cpu，只是为了让输出慢一点，不刷屏。
    }

    close(client_sock);
    close(server_sock);
    return;
}

void recv_coroutine(void* arg) {
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return;
    }
    printf("connect success\n");
    while (1) {
        int bytes_received = co_recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);
        //这里的sleep不会让出cpu，只是为了让输出慢一点，不刷屏。
        sleep(1);
    }

    close(client_sock);
    return;
}

int main() {
    log_set_level_from_env();
    PORT += rand() % 10000;
    Coroutine sender, receiver;
    coroutine_init(&sender, send_coroutine, "sender", STACKSIZE);
    coroutine_init(&receiver, recv_coroutine, "receiver", STACKSIZE);
    start_eventloop();
    while (1) coroutine_yield();
    return 0;
}
/*正确输出：

Server is listening on port 8080
connect success
accept success
Received: Message from server thread(1733905856)
Received: Message from server thread(1733905857)
Received: Message from server thread(1733905859)
Received: Message from server thread(1733905861)
Received: Message from server thread(1733905863)
Received: Message from server thread(1733905865)
Received: Message from server thread(1733905867)
Received: Message from server thread(1733905869)
Received: Message from server thread(1733905871)
Received: Message from server thread(1733905873)
Received: Message from server thread(1733905875)
...
*/
