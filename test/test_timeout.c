#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
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
        //每三秒发送一次消息
        //因为sleep还是阻塞模式，因此需要手动yield
        for (int i = 0; i < 30; i++) {
            usleep(100 * 1000);
            coroutine_yield();
        }
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
    //设置超时时间为1秒
    struct timeval rcv_timeout;
    rcv_timeout.tv_sec = 1;
    rcv_timeout.tv_usec = 0;
    if (co_setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &rcv_timeout, sizeof(rcv_timeout)) <
        0) {
        perror("setsockopt SO_RCVTIMEO");
        close(client_sock);
        return;
    }
    while (1) {
        int bytes_received = co_recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            continue;
        }
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);
    }

    close(client_sock);
    return;
}

int main() {
    log_set_level_from_env();
    srand(time(0));
    PORT += rand() % 10000;
    Coroutine sender, receiver;
    coroutine_init(&sender, send_coroutine, "sender", STACKSIZE);
    coroutine_init(&receiver, recv_coroutine, "receiver", STACKSIZE);
    start_eventloop();
    while (1) coroutine_yield();
    return 0;
}
/*正确输出：发每隔三秒一次，收超时一秒，所以应该是成功一次超时两次交替
Server is listening on port 4926
connect success
accept success
Received: Message from server thread(1734156641)
Receive failed: Resource temporarily unavailable
Receive failed: Resource temporarily unavailable
Received: Message from server thread(1734156644)
Receive failed: Resource temporarily unavailable
Receive failed: Resource temporarily unavailable
Received: Message from server thread(1734156647)
Receive failed: Resource temporarily unavailable
Receive failed: Resource temporarily unavailable
Received: Message from server thread(1734156650)
Receive failed: Resource temporarily unavailable
...
*/