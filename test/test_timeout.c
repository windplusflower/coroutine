/*
 * Copyright (c) 2025 windflower
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "coheader.h"

int PORT = 1000;
#define BUFFER_SIZE 1024

void* send_coroutine(const void* arg) {
    int server_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int client_sock;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return NULL;
    }

    if (listen(server_sock, 3) < 0) {
        perror("Listen failed");
        return NULL;
    }

    printf("Server is listening on port %d\n", PORT);
    if ((client_sock = accept(server_sock, (struct sockaddr*)&server_addr, &addr_len)) < 0) {
        perror("Accept failed");
        return NULL;
    }
    printf("accept success\n");
    while (1) {
        snprintf(buffer, BUFFER_SIZE, "Message from server thread(%ld)", time(0));
        if (send(client_sock, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            break;
        }
        // //每三秒发送一次消息
        sleep(3);
    }

    close(client_sock);
    close(server_sock);
    return NULL;
}

void* recv_coroutine(const void* arg) {
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return NULL;
    }
    printf("connect success\n");
    //设置超时时间为1秒
    struct timeval rcv_timeout;
    rcv_timeout.tv_sec = 1;
    rcv_timeout.tv_usec = 0;
    if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &rcv_timeout, sizeof(rcv_timeout)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        close(client_sock);
        return NULL;
    }
    while (1) {
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            continue;
        }
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);
    }

    close(client_sock);
    return NULL;
}

int main() {
    srand(time(0));
    PORT += rand() % 10000;
    enable_hook();
    coroutine_t sender, receiver;
    sender = coroutine_create(send_coroutine, "sender", 0);
    receiver = coroutine_create(recv_coroutine, "receiver", 0);
    coroutine_join(sender);
    coroutine_join(receiver);
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